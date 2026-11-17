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

fn trim<T: Copy + Eq>(mut s: &[T], ws: &[T]) -> &[T] {
    while s.first().is_some_and(|c| ws.contains(c)) {
        s = &s[1..];
    }
    while s.last().is_some_and(|c| ws.contains(c)) {
        s = &s[..s.len() - 1];
    }
    s
}
fn keep<T: Copy + Eq>(piece: &[T], ws: &[T], whitespace: Whitespace) -> &[T] {
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
pub struct ViewPair<'a> {
    pub key: &'a [u8],
    pub value: &'a [u8],
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
            if let Some(n) = value_start {
                out.push(ViewPair { key, value: &rest[n..] });
            } else {
                out.push(ViewPair { key, value: &[] });
                ok = false;
            }
        } else {
            out.push(ViewPair { key: &[], value: &[] });
            ok = false;
        }
    }
    (ok, out)
}
pub fn key_value_pairs(input: &[u8], kv: u8, pair: &[u8]) -> (bool, Vec<Pair>) {
    let (ok, views) = key_value_views(input, kv, pair);
    (ok, views.into_iter().map(|p| Pair { key: p.key.to_vec(), value: p.value.to_vec() }).collect())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn byte_and_utf16_edges() {
        assert!(split_bytes(b"", b",", Whitespace::Keep, Results::All).is_empty());
        assert_eq!(
            split_bytes(b"a\0b", b"\0", Whitespace::Keep, Results::All),
            [b"a".as_slice(), b"b"]
        );
        assert_eq!(
            split_bytes(&[0xff, b',', 0x80], b",", Whitespace::Keep, Results::All),
            [&[0xff][..], &[0x80][..]]
        );
        assert_eq!(
            split_utf16(&[0xd800, 44, 0xdc00], &[44], Whitespace::Keep, Results::All),
            [&[0xd800][..], &[0xdc00][..]]
        );
        assert_eq!(
            split_utf16(&[0x00a0, 65, 0x3000], &[44], Whitespace::Trim, Results::All),
            [&[65][..]]
        );
        assert_eq!(
            split_bytes(b"\x85x\x85", b",", Whitespace::Trim, Results::All),
            [b"\x85x\x85".as_slice()]
        );
    }
    #[test]
    fn separator_modes_views_and_empty_policy() {
        assert_eq!(
            split_bytes(b"red, green; blue:", b",:;", Whitespace::Trim, Results::NonEmpty),
            [b"red".as_slice(), b"green", b"blue"]
        );
        assert_eq!(
            split_bytes_substr(b"a::::b", b"::", Whitespace::Keep, Results::All),
            [b"a".as_slice(), b"".as_slice(), b"b"]
        );
        assert_eq!(split_bytes_substr(b"", b"", Whitespace::Trim, Results::All), [b"".as_slice()]);
        assert_eq!(
            split_bytes_substr(b"a,b", b"", Whitespace::Keep, Results::NonEmpty),
            [b"a,b".as_slice()]
        );
        let source = b"x,y";
        let parts = split_bytes(source, b",", Whitespace::Keep, Results::All);
        assert_eq!(parts[0].as_ptr(), source.as_ptr());
        assert_eq!(split_once(b"a:b:c", b":", true), Some((&b"a:b"[..], &b"c"[..])));
        assert_eq!(split_once(b"a=b:c", b":=", false), Some((&b"a=b"[..], &b"c"[..])));
    }
    #[test]
    fn key_value_partial_results_and_failure_policy() {
        let (ok, p) = key_value_views(b"key1,,key2:value2", b':', b",,");
        assert!(!ok);
        assert_eq!(
            p,
            [ViewPair { key: &[], value: &[] }, ViewPair { key: b"key2", value: b"value2" }]
        );
        let (ok, p) = key_value_views(b"key1:,key2:value2", b':', b",");
        assert!(!ok);
        assert_eq!(p[0], ViewPair { key: b"key1", value: &[] });
        assert_eq!(p[1].value, b"value2");
        assert_eq!(key_value_pairs(b"key1:value1,,key3:value3", b':', b",").1.len(), 2);
        assert_eq!(key_value_views(b":value1,key2:value2", b':', b",").0, true);
        assert_eq!(key_value_views(b"", b':', b",").1.len(), 0);
    }

    #[test]
    fn original_public_cases() {
        assert_eq!(split_bytes(b"", b",:;", Whitespace::Keep, Results::All), []);
        assert_eq!(
            split_bytes(b"hello, world", b"", Whitespace::Keep, Results::All),
            [b"hello, world".as_slice()]
        );
        assert_eq!(
            split_bytes(b"::,,;;", b",:;", Whitespace::Keep, Results::All),
            [b"".as_slice(); 7]
        );
        assert_eq!(split_bytes(b" ", b",", Whitespace::Trim, Results::All), [b"".as_slice()]);
        assert_eq!(split_bytes(b" ", b",", Whitespace::Trim, Results::NonEmpty), []);
        assert_eq!(
            split_bytes(b", ,", b",", Whitespace::Keep, Results::All),
            [b"".as_slice(), b" ", b""]
        );
        assert_eq!(
            split_bytes(b", ,", b",", Whitespace::Keep, Results::NonEmpty),
            [b" ".as_slice()]
        );
        assert_eq!(split_bytes(b", ,", b",", Whitespace::Trim, Results::All), [b"".as_slice(); 3]);
        assert_eq!(split_bytes(b", ,", b",", Whitespace::Trim, Results::NonEmpty), []);
        assert_eq!(
            split_bytes_substr(b"", b"DELIMITER", Whitespace::Trim, Results::All),
            [b"".as_slice()]
        );
        assert_eq!(
            split_bytes_substr(b"TEST", b"", Whitespace::Trim, Results::All),
            [b"TEST".as_slice()]
        );
        assert_eq!(
            split_bytes_substr(
                b"un DELIMITERdeux\tDELIMITERtrois\nDELIMITERquatre",
                b"DELIMITER",
                Whitespace::Keep,
                Results::All
            ),
            [b"un ".as_slice(), b"deux\t", b"trois\n", b"quatre"]
        );
        assert_eq!(
            split_bytes_substr(
                b"un DELIMITERdeux\tDELIMITERtrois\nDELIMITERquatre",
                b"DELIMITER",
                Whitespace::Trim,
                Results::All
            ),
            [b"un".as_slice(), b"deux", b"trois", b"quatre"]
        );
        assert_eq!(
            split_bytes_substr(
                b"unDELIMITERdeuxDELIMITERtroisDELIMITERDELIMITER",
                b"DELIMITER",
                Whitespace::Trim,
                Results::All
            ),
            [b"un".as_slice(), b"deux", b"trois", b"", b""]
        );
        assert_eq!(
            split_bytes_substr(
                b"unDELIMITERdeuxDELIMITERtroisDELIMITERDELIMITER",
                b"DELIMITER",
                Whitespace::Trim,
                Results::NonEmpty
            ),
            [b"un".as_slice(), b"deux", b"trois"]
        );
        assert_eq!(split_once(b"", b"", false), None);
        assert_eq!(split_once(b"a", b"", false), None);
        assert_eq!(split_once(b"a:b:c", b":", false), Some((&b"a"[..], &b"b:c"[..])));
        assert_eq!(split_once(b":b", b":", false), Some((&b""[..], &b"b"[..])));
        assert_eq!(split_once(b"a:", b":", false), Some((&b"a"[..], &b""[..])));
        assert_eq!(split_once(b"a:b=c", b":=", false), Some((&b"a"[..], &b"b=c"[..])));
        assert_eq!(split_once(b"a:b:c", b":", true), Some((&b"a:b"[..], &b"c"[..])));
        assert_eq!(
            split_bytes(b"\t  \ta\t ", b"\t", Whitespace::Keep, Results::All),
            [b"".as_slice(), b"  ", b"a", b" "]
        );
        assert_eq!(
            split_bytes(b"a\0a", b"\0", Whitespace::Keep, Results::NonEmpty),
            [b"a".as_slice(), b"a"]
        );
        assert_eq!(
            split_bytes(b"a\0", b"\0", Whitespace::Keep, Results::All),
            [b"a".as_slice(), b""]
        );
        assert_eq!(
            split_bytes(b"\0a\0\0a\0", b"\0", Whitespace::Keep, Results::All),
            [b"".as_slice(), b"a", b"", b"a", b""]
        );
    }

    #[test]
    fn original_key_value_assertions() {
        assert_eq!(key_value_views(b"", b':', b",").0, true);
        assert_eq!(
            key_value_views(b"key1:value1,,key3:value3", b':', b",,").1,
            [ViewPair { key: b"key1", value: b"value1,,key3:value3" }]
        );
        assert_eq!(
            key_value_views(b"key1 : value1", b':', b",").1,
            [ViewPair { key: b"key1 ", value: b" value1" }]
        );
        assert_eq!(
            key_value_views(b":value1,key2:value2", b':', b",").1,
            [ViewPair { key: b"", value: b"value1" }, ViewPair { key: b"key2", value: b"value2" }]
        );
        assert_eq!(key_value_views(b",key1:value1,key2:value2,", b':', b",").1.len(), 2);
        assert_eq!(key_value_views(b"key1:value1,,key3:value3", b':', b",").1.len(), 2);
        assert_eq!(
            key_value_views(b"key1:::value1,key2:value2", b':', b",").1,
            [
                ViewPair { key: b"key1", value: b"value1" },
                ViewPair { key: b"key2", value: b"value2" }
            ]
        );
        assert_eq!(key_value_views(b"key1:va:ue1,key2:value2", b':', b",").1[0].value, b"va:ue1");
        let (ok, p) = key_value_views(b"key1:value1 , key2:value2", b':', b",");
        assert!(ok);
        assert_eq!(
            p,
            [
                ViewPair { key: b"key1", value: b"value1" },
                ViewPair { key: b"key2", value: b"value2" }
            ]
        );
    }
}
