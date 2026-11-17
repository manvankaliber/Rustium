//! Safe Rust policy candidate for Chromium's byte and UTF-16 string splitting.
//! Inputs are slices so byte strings need not be UTF-8 and UTF-16 may contain
//! unpaired surrogates. Borrowed outputs cannot outlive their input.

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Whitespace {
    Keep,
    Trim,
}
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Results {
    All,
    NonEmpty,
}

const ASCII_WS: &[u8] = &[0x09, 0x0b, 0x0c, 0x20, 0x0a, 0x0d];
const UNICODE_WS: &[u16] = &[
    0x0009, 0x000b, 0x000c, 0x0020, 0x000a, 0x000d, 0x0085, 0x00a0, 0x1680, 0x2000, 0x2001, 0x2002,
    0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200a, 0x2028, 0x2029, 0x202f, 0x205f,
    0x3000,
];

fn trim<'a, T: Copy + Eq>(mut s: &'a [T], ws: &[T]) -> &'a [T] {
    while s.first().is_some_and(|c| ws.contains(c)) {
        s = &s[1..];
    }
    while s.last().is_some_and(|c| ws.contains(c)) {
        s = &s[..s.len() - 1];
    }
    s
}
fn keep<'a, T: Copy + Eq>(piece: &'a [T], ws: &[T], whitespace: Whitespace) -> &'a [T] {
    if whitespace == Whitespace::Trim {
        trim(piece, ws)
    } else {
        piece
    }
}
/// Splits at any separator code unit/byte. Empty input produces no tokens,
/// matching Chromium's generic SplitStringT implementation.
pub fn split_any<'a, T: Copy + Eq>(
    input: &'a [T],
    separators: &[T],
    whitespace: Whitespace,
    results: Results,
    ws: &[T],
) -> Vec<&'a [T]> {
    if input.is_empty() {
        return Vec::new();
    }
    let mut out = Vec::new();
    let mut start = 0;
    for (i, c) in input.iter().enumerate() {
        if separators.contains(c) {
            let p = keep(&input[start..i], ws, whitespace);
            if results == Results::All || !p.is_empty() {
                out.push(p);
            }
            start = i + 1;
        }
    }
    let p = keep(&input[start..], ws, whitespace);
    if results == Results::All || !p.is_empty() {
        out.push(p);
    }
    out
}

/// Splits on a complete delimiter. An empty delimiter returns the input as
/// one result, including the empty input; this differs from split_any.
pub fn split_substr<'a, T: Copy + Eq>(
    input: &'a [T],
    delimiter: &[T],
    whitespace: Whitespace,
    results: Results,
    ws: &[T],
) -> Vec<&'a [T]> {
    if delimiter.is_empty() {
        return vec![input];
    }
    let mut out = Vec::new();
    let mut start = 0;
    while start <= input.len() {
        let found = input[start..].windows(delimiter.len()).position(|w| w == delimiter);
        let end = found.map_or(input.len(), |n| start + n);
        let p = keep(&input[start..end], ws, whitespace);
        if results == Results::All || !p.is_empty() {
            out.push(p);
        }
        match found {
            Some(_) => start = end + delimiter.len(),
            None => break,
        }
    }
    out
}

pub fn split_bytes<'a>(
    input: &'a [u8],
    separators: &[u8],
    whitespace: Whitespace,
    results: Results,
) -> Vec<&'a [u8]> {
    split_any(input, separators, whitespace, results, ASCII_WS)
}
pub fn split_bytes_owned(
    input: &[u8],
    separators: &[u8],
    whitespace: Whitespace,
    results: Results,
) -> Vec<Vec<u8>> {
    split_bytes(input, separators, whitespace, results).into_iter().map(<[u8]>::to_vec).collect()
}
pub fn split_utf16<'a>(
    input: &'a [u16],
    separators: &[u16],
    whitespace: Whitespace,
    results: Results,
) -> Vec<&'a [u16]> {
    split_any(input, separators, whitespace, results, UNICODE_WS)
}
pub fn split_utf16_owned(
    input: &[u16],
    separators: &[u16],
    whitespace: Whitespace,
    results: Results,
) -> Vec<Vec<u16>> {
    split_utf16(input, separators, whitespace, results).into_iter().map(<[u16]>::to_vec).collect()
}
pub fn split_bytes_substr<'a>(
    input: &'a [u8],
    delimiter: &[u8],
    whitespace: Whitespace,
    results: Results,
) -> Vec<&'a [u8]> {
    split_substr(input, delimiter, whitespace, results, ASCII_WS)
}
pub fn split_bytes_substr_owned(
    input: &[u8],
    delimiter: &[u8],
    whitespace: Whitespace,
    results: Results,
) -> Vec<Vec<u8>> {
    split_bytes_substr(input, delimiter, whitespace, results)
        .into_iter()
        .map(<[u8]>::to_vec)
        .collect()
}
pub fn split_utf16_substr<'a>(
    input: &'a [u16],
    delimiter: &[u16],
    whitespace: Whitespace,
    results: Results,
) -> Vec<&'a [u16]> {
    split_substr(input, delimiter, whitespace, results, UNICODE_WS)
}
pub fn split_utf16_substr_owned(
    input: &[u16],
    delimiter: &[u16],
    whitespace: Whitespace,
    results: Results,
) -> Vec<Vec<u16>> {
    split_utf16_substr(input, delimiter, whitespace, results)
        .into_iter()
        .map(<[u16]>::to_vec)
        .collect()
}

pub fn split_once<'a>(
    input: &'a [u8],
    separators: &[u8],
    reverse: bool,
) -> Option<(&'a [u8], &'a [u8])> {
    let i = if reverse {
        input.iter().rposition(|c| separators.contains(c))
    } else {
        input.iter().position(|c| separators.contains(c))
    }?;
    Some((&input[..i], &input[i + 1..]))
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ViewOrigin {
    /// The field is a slice of the caller's input, including a zero-length slice.
    Input,
    /// The C++ implementation left this default-constructed field untouched.
    DefaultEmpty,
}
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct ViewField<'a> {
    pub bytes: &'a [u8],
    pub offset: usize,
    pub origin: ViewOrigin,
}
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct ViewPair<'a> {
    pub key: ViewField<'a>,
    pub value: ViewField<'a>,
}
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Pair {
    pub key: Vec<u8>,
    pub value: Vec<u8>,
}

/// Mirrors Chromium's partial-result rule: each nonempty pair segment appends
/// a tuple, malformed segments append an empty tuple, and parsing continues.
pub fn key_value_views<'a>(input: &'a [u8], kv: u8, pair: &[u8]) -> (bool, Vec<ViewPair<'a>>) {
    let parts = split_bytes_substr(input, pair, Whitespace::Trim, Results::NonEmpty);
    let mut ok = true;
    let mut out = Vec::with_capacity(parts.len());
    for part in parts {
        if let Some(i) = part.iter().position(|c| *c == kv) {
            let key = &part[..i];
            let rest = &part[i..];
            let value_start = rest.iter().position(|c| *c != kv);
            let key_offset = key.as_ptr() as usize - input.as_ptr() as usize;
            if let Some(n) = value_start {
                let value = &rest[n..];
                let value_offset = value.as_ptr() as usize - input.as_ptr() as usize;
                out.push(ViewPair {
                    key: ViewField { bytes: key, offset: key_offset, origin: ViewOrigin::Input },
                    value: ViewField {
                        bytes: value,
                        offset: value_offset,
                        origin: ViewOrigin::Input,
                    },
                });
            } else {
                out.push(ViewPair {
                    key: ViewField { bytes: key, offset: key_offset, origin: ViewOrigin::Input },
                    value: ViewField { bytes: &[], offset: 0, origin: ViewOrigin::DefaultEmpty },
                });
                ok = false;
            }
        } else {
            out.push(ViewPair {
                key: ViewField { bytes: &[], offset: 0, origin: ViewOrigin::DefaultEmpty },
                value: ViewField { bytes: &[], offset: 0, origin: ViewOrigin::DefaultEmpty },
            });
            ok = false;
        }
    }
    (ok, out)
}
/// Output-parameter form matching the C++ clearing contract.
pub fn key_value_views_into<'a>(
    input: &'a [u8],
    kv: u8,
    pair: &[u8],
    output: &mut Vec<ViewPair<'a>>,
) -> bool {
    output.clear();
    let (success, parsed) = key_value_views(input, kv, pair);
    *output = parsed;
    success
}
pub fn key_value_pairs(input: &[u8], kv: u8, pair: &[u8]) -> (bool, Vec<Pair>) {
    let (ok, views) = key_value_views(input, kv, pair);
    (
        ok,
        views
            .into_iter()
            .map(|p| Pair { key: p.key.bytes.to_vec(), value: p.value.bytes.to_vec() })
            .collect(),
    )
}
/// Owned output-parameter form; clears prior contents before parsing.
pub fn key_value_pairs_into(input: &[u8], kv: u8, pair: &[u8], output: &mut Vec<Pair>) -> bool {
    output.clear();
    let (success, parsed) = key_value_pairs(input, kv, pair);
    *output = parsed;
    success
}

#[cfg(test)]
mod tests {
    use super::*;

    fn any(i: &[u8], d: &[u8], w: Whitespace, r: Results, e: &[&[u8]]) {
        assert_eq!(split_bytes(i, d, w, r), e);
        assert_eq!(split_bytes_owned(i, d, w, r), e.iter().map(|x| x.to_vec()).collect::<Vec<_>>());
    }
    fn substr(i: &[u8], d: &[u8], w: Whitespace, r: Results, e: &[&[u8]]) {
        assert_eq!(split_bytes_substr(i, d, w, r), e);
        assert_eq!(
            split_bytes_substr_owned(i, d, w, r),
            e.iter().map(|x| x.to_vec()).collect::<Vec<_>>()
        );
    }
    fn kv(i: &[u8], k: u8, d: &[u8], ok: bool, e: &[(&[u8], &[u8])]) {
        let (vo, v) = key_value_views(i, k, d);
        let (oo, o) = key_value_pairs(i, k, d);
        assert_eq!(vo, ok);
        assert_eq!(oo, ok);
        assert_eq!(v.iter().map(|x| (x.key.bytes, x.value.bytes)).collect::<Vec<_>>(), e);
        assert_eq!(
            o,
            e.iter().map(|(a, b)| Pair { key: a.to_vec(), value: b.to_vec() }).collect::<Vec<_>>()
        );
    }
    #[test]
    fn original_split_any_all_owned_and_borrowed() {
        any(b"", b",:;", Whitespace::Keep, Results::All, &[]);
        any(b"hello, world", b"", Whitespace::Keep, Results::All, &[b"hello, world"]);
        any(
            b"::,,;;",
            b",:;",
            Whitespace::Keep,
            Results::All,
            &[b"", b"", b"", b"", b"", b"", b""],
        );
        any(
            b"red, green; blue:",
            b",:;",
            Whitespace::Trim,
            Results::NonEmpty,
            &[b"red", b"green", b"blue"],
        );
        any(
            b"  red green   \tblue\n",
            b" \t\n",
            Whitespace::Trim,
            Results::NonEmpty,
            &[b"red", b"green", b"blue"],
        );
        any(b" red ", b" ", Whitespace::Trim, Results::All, &[b"", b"red", b""]);
        any(b"", b",", Whitespace::Keep, Results::All, &[]);
        any(b"", b",", Whitespace::Keep, Results::NonEmpty, &[]);
        any(b" ", b",", Whitespace::Trim, Results::All, &[b""]);
        any(b" ", b",", Whitespace::Trim, Results::NonEmpty, &[]);
        any(b", ,", b",", Whitespace::Keep, Results::All, &[b"", b" ", b""]);
        any(b", ,", b",", Whitespace::Keep, Results::NonEmpty, &[b" "]);
        any(b", ,", b",", Whitespace::Trim, Results::All, &[b"", b"", b""]);
        any(b", ,", b",", Whitespace::Trim, Results::NonEmpty, &[]);
        any(b"   ", b"*", Whitespace::Keep, Results::All, &[b"   "]);
        any(b"\t  \ta\t ", b"\t", Whitespace::Keep, Results::All, &[b"", b"  ", b"a", b" "]);
        any(b"\ta\t\nb\tcc", b"\n", Whitespace::Keep, Results::All, &[b"\ta\t", b"b\tcc"]);
        let rows: &[(&[u8], &[&[u8]])] = &[
            (b"a", &[b"a"]),
            (b" ", &[]),
            (b" a", &[b"a"]),
            (b" ab ", &[b"ab"]),
            (b" ab c", &[b"ab", b"c"]),
            (b" ab c ", &[b"ab", b"c"]),
            (b" ab cd", &[b"ab", b"cd"]),
            (b" ab cd ", &[b"ab", b"cd"]),
            (b" \ta\t", &[b"a"]),
            (b" b\ta\t", &[b"b", b"a"]),
            (b" b\tat", &[b"b", b"at"]),
            (b"b\tat", &[b"b", b"at"]),
            (b"b\t at", &[b"b", b"at"]),
        ];
        for (i, e) in rows {
            any(i, ASCII_WS, Whitespace::Keep, Results::NonEmpty, e);
        }
        any(b"a", b"\0", Whitespace::Keep, Results::NonEmpty, &[b"a"]);
        any(b"a\0", b"\0", Whitespace::Keep, Results::NonEmpty, &[b"a"]);
        any(b"a\0a", b"\0", Whitespace::Keep, Results::NonEmpty, &[b"a", b"a"]);
        any(b"\0a\0\0a\0", b"\0", Whitespace::Keep, Results::NonEmpty, &[b"a", b"a"]);
        any(b"a", b"\0", Whitespace::Keep, Results::All, &[b"a"]);
        any(b"a\0", b"\0", Whitespace::Keep, Results::All, &[b"a", b""]);
        any(b"a\0a", b"\0", Whitespace::Keep, Results::All, &[b"a", b"a"]);
        any(b"\0a\0\0a\0", b"\0", Whitespace::Keep, Results::All, &[b"", b"a", b"", b"a", b""]);
    }
    #[test]
    fn original_substr_all_owned_and_borrowed() {
        substr(b"", b"DELIMITER", Whitespace::Trim, Results::All, &[b""]);
        substr(b"TEST", b"", Whitespace::Trim, Results::All, &[b"TEST"]);
        substr(
            b"alongwordwithnodelimiter",
            b"DELIMITER",
            Whitespace::Trim,
            Results::All,
            &[b"alongwordwithnodelimiter"],
        );
        substr(
            b"DELIMITERDELIMITERDELIMITERoneDELIMITERtwoDELIMITERthree",
            b"DELIMITER",
            Whitespace::Trim,
            Results::All,
            &[b"", b"", b"", b"one", b"two", b"three"],
        );
        substr(
            b"unoDELIMITERDELIMITERDELIMITERdosDELIMITERtresDELIMITERDELIMITERcuatro",
            b"DELIMITER",
            Whitespace::Trim,
            Results::All,
            &[b"uno", b"", b"", b"dos", b"tres", b"", b"cuatro"],
        );
        substr(
            b"unDELIMITERdeuxDELIMITERtroisDELIMITERquatreDELIMITERDELIMITERDELIMITER",
            b"DELIMITER",
            Whitespace::Trim,
            Results::All,
            &[b"un", b"deux", b"trois", b"quatre", b"", b"", b""],
        );
        substr(
            b"un DELIMITERdeux\tDELIMITERtrois\nDELIMITERquatre",
            b"DELIMITER",
            Whitespace::Keep,
            Results::All,
            &[b"un ", b"deux\t", b"trois\n", b"quatre"],
        );
        substr(
            b"un DELIMITERdeux\tDELIMITERtrois\nDELIMITERquatre",
            b"DELIMITER",
            Whitespace::Trim,
            Results::All,
            &[b"un", b"deux", b"trois", b"quatre"],
        );
        substr(
            b"unDELIMITERdeuxDELIMITERtroisDELIMITERDELIMITER",
            b"DELIMITER",
            Whitespace::Trim,
            Results::All,
            &[b"un", b"deux", b"trois", b"", b""],
        );
        substr(
            b"unDELIMITERdeuxDELIMITERtroisDELIMITERDELIMITER",
            b"DELIMITER",
            Whitespace::Trim,
            Results::NonEmpty,
            &[b"un", b"deux", b"trois"],
        );
        substr(b"abcDELIMITERdef", b"DELIMITER", Whitespace::Keep, Results::All, &[b"abc", b"def"]);
    }
    #[test]
    fn original_split_once_permutations() {
        for d in [b":".as_slice(), b":=", b"=:"] {
            assert_eq!(split_once(b"a:b:c", d, false), Some((&b"a"[..], &b"b:c"[..])));
            assert_eq!(split_once(b"a:b:c", d, true), Some((&b"a:b"[..], &b"c"[..])));
        }
        for d in [b":=".as_slice(), b"=:"] {
            assert_eq!(split_once(b"a:b=c", d, false), Some((&b"a"[..], &b"b=c"[..])));
            assert_eq!(split_once(b"a=b:c", d, false), Some((&b"a"[..], &b"b:c"[..])));
            assert_eq!(split_once(b"a:b=c", d, true), Some((&b"a:b"[..], &b"c"[..])));
            assert_eq!(split_once(b"a=b:c", d, true), Some((&b"a=b"[..], &b"c"[..])));
        }
        assert_eq!(split_once(b"", b"", false), None);
        assert_eq!(split_once(b"a", b"", false), None);
        assert_eq!(split_once(b"ab", b"", true), None);
        assert_eq!(split_once(b"a:", b":", false), Some((&b"a"[..], &b""[..])));
        assert_eq!(split_once(b":b", b":", false), Some((&b""[..], &b"b"[..])));
    }
    #[test]
    fn original_key_value_cases_both_typed_variants() {
        kv(b"", b':', b",", true, &[]);
        kv(b"key1,,key2:value2", b':', b",,", false, &[(b"", b""), (b"key2", b"value2")]);
        kv(b"key1:value1,,key3:value3", b':', b",,,", true, &[(b"key1", b"value1,,key3:value3")]);
        kv(b"key1 : value1", b':', b",", true, &[(b"key1 ", b" value1")]);
        let a = b"a ?!@#$%^&*()_+:/{}\\\t\nb";
        let mut s = a.to_vec();
        s.push(b'X');
        s.extend_from_slice(a);
        s.extend_from_slice(b"XY");
        s.extend_from_slice(a);
        s.extend_from_slice(b"YX");
        s.extend_from_slice(a);
        kv(&s, b'X', b"XY", true, &[(a, a), (b"a ?!@#$%^&*()_+:/{}\\\t\nbY", a)]);
        kv(b"key1,key2:value2", b':', b",", false, &[(b"", b""), (b"key2", b"value2")]);
        kv(b":value1,key2:value2", b':', b",", true, &[(b"", b"value1"), (b"key2", b"value2")]);
        kv(
            b",key1:value1,key2:value2,",
            b':',
            b",",
            true,
            &[(b"key1", b"value1"), (b"key2", b"value2")],
        );
        kv(
            b"key1:value1,,key3:value3",
            b':',
            b",",
            true,
            &[(b"key1", b"value1"), (b"key3", b"value3")],
        );
        kv(b"key1:,key2:value2", b':', b",", false, &[(b"key1", b""), (b"key2", b"value2")]);
        kv(
            b"key1:value1 , key2:value2",
            b':',
            b",",
            true,
            &[(b"key1", b"value1"), (b"key2", b"value2")],
        );
        kv(
            b"key1:::value1,key2:value2",
            b':',
            b",",
            true,
            &[(b"key1", b"value1"), (b"key2", b"value2")],
        );
        kv(
            b"key1:va:ue1,key2:value2",
            b':',
            b",",
            true,
            &[(b"key1", b"va:ue1"), (b"key2", b"value2")],
        );
    }
    #[test]
    fn malformed_default_empty_differs_from_borrowed_empty() {
        let input = b":value,key:,tail:v";
        let (ok, p) = key_value_views(input, b':', b",");
        assert!(!ok);
        assert_eq!(p[0].key.origin, ViewOrigin::Input);
        assert_eq!(p[0].key.offset, 0);
        assert!(p[0].key.bytes.is_empty());
        assert_eq!(p[1].value.origin, ViewOrigin::DefaultEmpty);
        assert_eq!(p[2].key.origin, ViewOrigin::DefaultEmpty);
        assert_eq!(p[2].value.origin, ViewOrigin::DefaultEmpty);
        assert_eq!(&input[p[0].key.offset..p[0].key.offset + p[0].key.bytes.len()], p[0].key.bytes);
        let mut reused = vec![Pair { key: b"stale".to_vec(), value: b"stale".to_vec() }];
        let mut views = vec![ViewPair { key: p[0].key, value: p[0].value }];
        assert!(!key_value_views_into(input, b':', b",", &mut views));
        assert_eq!(views, p);
        assert!(!key_value_pairs_into(input, b':', b",", &mut reused));
        assert_eq!(reused.len(), 3);
    }
    #[test]
    fn utf16_bytes_and_owned_variants() {
        assert_eq!(
            split_bytes(b"a\0b", b"\0", Whitespace::Keep, Results::All),
            [b"a".as_slice(), b"b"]
        );
        assert_eq!(
            split_bytes(&[0xff, b',', 0x80], b",", Whitespace::Keep, Results::All),
            [&[0xff][..], &[0x80][..]]
        );
        assert_eq!(
            split_bytes_owned(&[0xff, b',', 0x80], b",", Whitespace::Keep, Results::All),
            [vec![0xff], vec![0x80]]
        );
        assert_eq!(
            split_utf16(&[0xd800, 44, 0xdc00], &[44], Whitespace::Keep, Results::All),
            [&[0xd800][..], &[0xdc00][..]]
        );
        assert_eq!(
            split_utf16_owned(&[0xd800, 44, 0xdc00], &[44], Whitespace::Keep, Results::All),
            [vec![0xd800], vec![0xdc00]]
        );
        assert_eq!(
            split_utf16(&[0x00a0, 65, 0x3000], &[44], Whitespace::Trim, Results::All),
            [&[65][..]]
        );
        assert_eq!(
            split_utf16_substr(
                &[65, 0x3000, 44, 0x00a0, 66],
                &[44],
                Whitespace::Trim,
                Results::All
            ),
            [&[65][..], &[66][..]]
        );
        assert_eq!(
            split_utf16_substr_owned(&[65, 44, 66], &[44], Whitespace::Keep, Results::All),
            [vec![65], vec![66]]
        );
        assert_eq!(
            split_bytes(b"\x85x\x85", b",", Whitespace::Trim, Results::All),
            [b"\x85x\x85".as_slice()]
        );
    }
}
