// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![forbid(unsafe_code)]

#[derive(Clone, Default)]
pub struct Rule {
    pub kind: u8,
    pub scheme: Vec<u8>,
    pub host: Vec<u8>,
    pub port: i32,
    pub ip: Vec<u8>,
    pub bits: usize,
}
pub struct Authority {
    pub host: Vec<u8>,
    pub port: i32,
    pub valid: bool,
}
pub trait Services {
    fn ip(&self, raw: &[u8]) -> Vec<u8>;
    fn canonical(&self, ip: &[u8]) -> Vec<u8>;
    fn authority(&self, raw: &[u8]) -> Authority;
}
pub struct Url<'a> {
    pub scheme: &'a [u8],
    pub host: &'a [u8],
    pub port: i32,
    pub is_ip: bool,
    pub windows: bool,
    pub unspecified: bool,
}
pub fn trim(v: &[u8]) -> &[u8] {
    let white = |c: &u8| b" \t\r\n\x0b\x0c".contains(c);
    let start = v.iter().position(|c| !white(c)).unwrap_or(v.len());
    let end = v.iter().rposition(|c| !white(c)).map_or(start, |i| i + 1);
    &v[start..end]
}
fn number(v: &[u8], max: u32) -> Option<u32> {
    if v.is_empty() {
        return None;
    }
    v.iter().try_fold(0u32, |n, c| {
        if !c.is_ascii_digit() {
            return None;
        }
        n.checked_mul(10)?.checked_add(u32::from(c - b'0')).filter(|n| *n <= max)
    })
}
pub fn parse(raw: &[u8], proxy: bool, svc: &impl Services) -> Rule {
    let mut raw = trim(raw);
    if proxy && raw.eq_ignore_ascii_case(b"<local>") {
        return special(4);
    }
    if proxy && raw.eq_ignore_ascii_case(b"<-loopback>") {
        return special(5);
    }
    let mut scheme = Vec::new();
    if let Some(i) = raw.windows(3).position(|s| s == b"://") {
        if i == 0 {
            return Rule::default();
        }
        scheme = raw[..i].to_vec();
        raw = &raw[i + 3..];
    }
    if raw.is_empty() {
        return Rule::default();
    }
    if raw.contains(&b'/') {
        let parts: Vec<_> = raw.split(|c| *c == b'/').collect();
        if parts.len() != 2 {
            return Rule::default();
        }
        let ip = svc.ip(trim(parts[0]));
        if ip.is_empty() {
            return Rule::default();
        }
        let Some(bits) = number(trim(parts[1]), (ip.len() * 8) as u32) else {
            return Rule::default();
        };
        return Rule {
            kind: 3,
            scheme,
            host: raw.to_vec(),
            ip,
            bits: bits as usize,
            ..Rule::default()
        };
    }
    let authority = svc.authority(raw);
    if authority.valid {
        let ip = svc.ip(&authority.host);
        if !ip.is_empty() {
            return ip_host(&scheme, &svc.canonical(&ip), authority.port.max(0));
        }
    }
    let mut port = -1;
    if let Some(i) = raw.iter().rposition(|c| *c == b':') {
        let Some(n) = number(&raw[i + 1..], 65535) else {
            return Rule::default();
        };
        port = n as i32;
        raw = &raw[..i];
    }
    let mut host = raw.to_vec();
    if host.starts_with(b".") {
        host.insert(0, b'*');
    }
    hostname(&scheme, &host, port)
}
pub fn hostname(scheme: &[u8], host: &[u8], port: i32) -> Rule {
    Rule {
        kind: 1,
        scheme: scheme.to_ascii_lowercase(),
        host: host.to_ascii_lowercase(),
        port,
        ..Rule::default()
    }
}
pub fn ip_host(scheme: &[u8], canonical: &[u8], port: i32) -> Rule {
    let mut host = canonical.to_vec();
    if host.contains(&b':') {
        host.insert(0, b'[');
        host.push(b']');
    }
    Rule { kind: 2, scheme: scheme.to_ascii_lowercase(), host, port, ..Rule::default() }
}
pub fn special(kind: u8) -> Rule {
    Rule { kind, ..Rule::default() }
}
pub fn suffix(rule: &Rule) -> Rule {
    let mut r = rule.clone();
    if !r.host.starts_with(b"*") {
        r.host.insert(0, b'*');
    }
    r
}
fn c_string(v: &[u8]) -> &[u8] {
    &v[..v.iter().position(|c| *c == 0).unwrap_or(v.len())]
}
pub fn serialize(r: &Rule) -> Vec<u8> {
    if r.kind == 3 {
        return r.host.clone();
    }
    if r.kind == 4 {
        return b"<local>".to_vec();
    }
    if r.kind == 5 {
        return b"<-loopback>".to_vec();
    }
    let mut s = Vec::new();
    if !r.scheme.is_empty() {
        s.extend_from_slice(c_string(&r.scheme));
        s.extend_from_slice(b"://");
    }
    s.extend_from_slice(&r.host);
    if (r.kind == 1 && r.port != -1) || (r.kind == 2 && r.port != 0) {
        s.push(b':');
        s.extend_from_slice(r.port.to_string().as_bytes());
    }
    s
}
// ICU UTF-8 iteration rejects malformed scalars; malformed bytes never match,
// but wildcard scanning consumes the same malformed subsequence length.
fn next(v: &[u8], i: &mut usize) -> i32 {
    if *i == v.len() {
        return -1;
    }
    let rest = &v[*i..];
    if rest[0].is_ascii() {
        *i += 1;
        return i32::from(rest[0]);
    }
    match std::str::from_utf8(rest) {
        Ok(s) => {
            let c = s.chars().next().unwrap();
            *i += c.len_utf8();
            c as i32
        }
        Err(e) if e.valid_up_to() > 0 => {
            let c = std::str::from_utf8(&rest[..e.valid_up_to()]).unwrap().chars().next().unwrap();
            *i += c.len_utf8();
            c as i32
        }
        Err(e) => {
            *i += e.error_len().unwrap_or(rest.len());
            -1
        }
    }
}
pub fn pattern(eval: &[u8], pat: &[u8]) -> bool {
    let (mut p, mut s) = (0, 0);
    loop {
        let mut max = 0i64;
        let mut star = false;
        while p < pat.len() && matches!(pat[p], b'?' | b'*') {
            star |= pat[p] == b'*';
            max += 1;
            p += 1;
        }
        if star {
            max = -1;
        }
        let (p0, mut s0) = (p, s);
        let mut escape = false;
        loop {
            if p == pat.len() {
                if s == eval.len() {
                    return true;
                }
            } else {
                if !escape && matches!(pat[p], b'*' | b'?') {
                    break;
                }
                if !escape && pat[p] == b'\\' {
                    escape = true;
                    p += 1;
                    continue;
                }
                escape = false;
                if s == eval.len() {
                    return false;
                }
                let (mut pn, mut sn) = (p, s);
                let c = next(pat, &mut pn);
                if c == next(eval, &mut sn) && c != -1 {
                    p = pn;
                    s = sn;
                    continue;
                }
            }
            if max == 0 {
                return false;
            }
            max -= 1;
            p = p0;
            next(eval, &mut s0);
            s = s0;
        }
    }
}
fn mapped(ip: &[u8]) -> bool {
    ip.len() == 16 && ip[..10] == [0; 10] && ip[10..12] == [255; 2]
}
fn mapped_v4(ip: &[u8]) -> Vec<u8> {
    let mut out = vec![0; 10];
    out.extend_from_slice(&[255, 255]);
    out.extend_from_slice(ip);
    out
}
pub fn prefix(ip: &[u8], block: &[u8], bits: usize) -> bool {
    if !matches!(ip.len(), 4 | 16) || !matches!(block.len(), 4 | 16) || bits > block.len() * 8 {
        return false;
    }
    if ip.len() != block.len() {
        if ip.len() == 4 {
            return prefix(&mapped_v4(ip), block, bits);
        }
        return prefix(ip, &mapped_v4(block), 96 + bits);
    }
    let bytes = bits / 8;
    let tail = bits % 8;
    ip[..bytes] == block[..bytes]
        && (tail == 0 || (ip[bytes] >> (8 - tail)) == (block[bytes] >> (8 - tail)))
}
pub fn implicit(url: &Url<'_>, svc: &impl Services) -> bool {
    let host = url.host.strip_prefix(b"[").and_then(|s| s.strip_suffix(b"]")).unwrap_or(url.host);
    let ip = svc.ip(host);
    if ip.len() == 4 && ip[0] == 127 || ip.len() == 16 && ip[..15] == [0; 15] && ip[15] == 1 {
        return true;
    }
    if ip.is_empty() {
        let name = host.strip_suffix(b".").unwrap_or(host);
        if name.eq_ignore_ascii_case(b"localhost")
            || (name.len() >= 10 && name[name.len() - 10..].eq_ignore_ascii_case(b".localhost"))
        {
            return true;
        }
    }
    if url.host.starts_with(b"[::ffff") && mapped(&ip) && ip[12] == 127 {
        return true;
    }
    if (url.host.starts_with(b"169.254.") || url.host.starts_with(b"["))
        && ((ip.len() == 4 && ip[..2] == [169, 254])
            || (ip.len() == 16 && ip[0] == 254 && ip[1] & 192 == 128)
            || (mapped(&ip) && ip[12..14] == [169, 254]))
    {
        return true;
    }
    if (url.host.starts_with(b"0.") || url.host.starts_with(b"["))
        && url.unspecified
        && (!ip.is_empty() && ip.iter().all(|b| *b == 0)
            || mapped(&ip) && ip[12..].iter().all(|b| *b == 0))
    {
        return true;
    }
    url.windows && (url.host == b"loopback" || url.host == b"loopback.")
}
pub fn evaluate(r: &Rule, url: &Url<'_>, svc: &impl Services) -> u8 {
    match r.kind {
        1 | 2 => {
            let wildcard_port = if r.kind == 1 { -1 } else { 0 };
            u8::from(
                (r.port == wildcard_port || r.port == url.port)
                    && (r.scheme.is_empty() || r.scheme == url.scheme)
                    && pattern(url.host, &r.host),
            )
        }
        3 => {
            if !url.is_ip || !r.scheme.is_empty() && r.scheme != url.scheme {
                return 0;
            }
            let host =
                url.host.strip_prefix(b"[").and_then(|s| s.strip_suffix(b"]")).unwrap_or(url.host);
            let ip = svc.ip(host);
            if ip.is_empty() {
                return 0;
            }
            // Match IPAddressMatchesPrefix's checked public preconditions when
            // a directly constructed IPBlock has invalid prefix arguments.
            assert!(matches!(r.ip.len(), 4 | 16) && r.bits <= r.ip.len() * 8);
            u8::from(prefix(&ip, &r.ip, r.bits))
        }
        4 => u8::from(!url.host.contains(&b'.') && !url.is_ip),
        5 if implicit(url, svc) => 2,
        _ => 0,
    }
}
pub fn proxy_result(result: u8, reverse: bool, implicit: impl FnOnce() -> bool) -> bool {
    match result {
        1 => !reverse,
        2 => reverse,
        _ => implicit() || reverse,
    }
}
#[derive(Default)]
pub struct Matcher {
    pub rules: Vec<Option<Rule>>,
}
impl Matcher {
    pub fn insert(&mut self, index: usize, r: Option<Rule>) {
        self.rules.insert(index, r);
    }
    pub fn replace(&mut self, index: usize, r: Option<Rule>) {
        self.rules[index] = r;
    }
}
pub struct Evaluation {
    pub result: u8,
    pub index: usize,
    pub custom: bool,
}
pub struct TextStep {
    pub bytes: Vec<u8>,
    pub index: usize,
    pub custom: bool,
}
impl Matcher {
    pub fn step(&self, start: usize, url: &Url<'_>, svc: &impl Services) -> Evaluation {
        for i in (0..start.min(self.rules.len())).rev() {
            let Some(r) = &self.rules[i] else {
                return Evaluation { result: 0, index: i, custom: true };
            };
            let result = evaluate(r, url, svc);
            if result != 0 {
                return Evaluation { result, index: i, custom: false };
            }
        }
        Evaluation { result: 0, index: 0, custom: false }
    }
    pub fn text_step(&self, start: usize, previous: &[u8], has_previous: bool) -> TextStep {
        let mut bytes = Vec::new();
        if has_previous {
            bytes.extend_from_slice(previous);
            bytes.push(b';');
        }
        for (i, rule) in self.rules.iter().enumerate().skip(start) {
            let Some(r) = rule else {
                return TextStep { bytes, index: i, custom: true };
            };
            bytes.extend(serialize(r));
            bytes.push(b';');
        }
        TextStep { bytes, index: self.rules.len(), custom: false }
    }
    pub fn compare_step(
        &self,
        other: &Self,
        start: usize,
        a: &[u8],
        b: &[u8],
        has_previous: bool,
    ) -> Evaluation {
        if (start == 0 && !has_previous && self.rules.len() != other.rules.len())
            || (has_previous && a != b)
        {
            return Evaluation { result: 2, index: 0, custom: false };
        }
        for i in start..self.rules.len() {
            match (&self.rules[i], &other.rules[i]) {
                (Some(x), Some(y)) => {
                    if serialize(x) != serialize(y) {
                        return Evaluation { result: 2, index: i, custom: false };
                    }
                }
                _ => return Evaluation { result: 0, index: i, custom: true },
            }
        }
        Evaluation { result: 1, index: self.rules.len(), custom: false }
    }
}
pub fn tokens(raw: &[u8]) -> impl Iterator<Item = &[u8]> {
    raw.split(|c| matches!(c, b',' | b';')).filter(|s| !s.is_empty())
}

pub fn port_number(raw: &[u8]) -> Option<i32> {
    number(raw, 65535).map(|n| n as i32)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn unicode_pattern_and_escape_contract() {
        assert!(pattern("\u{e9}".as_bytes(), b"?"));
        assert!(pattern("\u{e9}".as_bytes(), "\u{e9}".as_bytes()));
        assert!(!pattern("\u{e9}x".as_bytes(), b"?"));
        assert!(pattern(b"", b"?")); // Chromium '?' consumes zero or one.
        assert!(pattern(b"*", b"\\*"));
        assert!(pattern(b"", b"\\"));
        assert!(!pattern(&[255], &[255]));
        assert!(pattern(&[255], b"*"));
        assert!(!pattern(&[195, 169, 255], &[195, 169, 255]));
        assert!(pattern(&[195, 169, 255], &[195, 169, b'*']));
        let mut pos = 0;
        assert_eq!(next(b"", &mut pos), -1);
    }
    #[test]
    fn prefix_family_conversion_and_boundaries() {
        assert!(!prefix(&[], &[0; 4], 0));
        assert!(!prefix(&[0; 4], &[0; 4], 33));
        assert!(prefix(&[192, 168, 1, 2], &[192, 168, 0, 0], 16));
        assert!(!prefix(&[192, 169, 1, 2], &[192, 168, 0, 0], 16));
        assert!(prefix(&mapped_v4(&[127, 0, 0, 1]), &[127, 0, 0, 0], 8));
        assert!(prefix(&[127, 0, 0, 1], &mapped_v4(&[127, 0, 0, 0]), 104));
    }
    struct NoIp;
    impl Services for NoIp {
        fn ip(&self, _: &[u8]) -> Vec<u8> {
            vec![]
        }
        fn canonical(&self, _: &[u8]) -> Vec<u8> {
            vec![]
        }
        fn authority(&self, _: &[u8]) -> Authority {
            Authority { host: vec![], port: -1, valid: false }
        }
    }
    #[test]
    fn non_windows_and_reentrant_custom_serialization_contract() {
        let url = Url {
            host: b"loopback",
            scheme: b"http",
            port: 80,
            is_ip: false,
            windows: false,
            unspecified: true,
        };
        assert!(!implicit(&url, &NoIp));
        assert_eq!(evaluate(&Rule::default(), &url, &NoIp), 0);
        assert_eq!(port_number(b"99999999999999999999999999999"), None);
        let a = Matcher { rules: vec![None] };
        let b = Matcher { rules: vec![None] };
        assert!(a.compare_step(&b, 0, b"", b"", false).custom);
        assert_eq!(a.compare_step(&b, 1, b"x", b"x", true).result, 1);
        assert_eq!(a.compare_step(&b, 1, b"x", b"y", true).result, 2);
    }
}
