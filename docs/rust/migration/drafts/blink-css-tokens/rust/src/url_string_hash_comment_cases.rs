struct NextExpected {
    kind: Kind,
    value: Option<&'static [u16]>,
    delimiter: Option<u16>,
    hash_is_id: bool,
}
struct NextCase {
    label: &'static str,
    input: &'static [u16],
    expected: &'static [NextExpected],
}
static NEXT_ORIGINAL_CASES: &[NextCase] = &[
    NextCase {
        label: "UrlToken:424",
        input: &[117, 114, 108, 40, 102, 111, 111, 46, 103, 105, 102, 41],
        expected: &[NextExpected {
            kind: Kind::Url,
            value: Some(&[102, 111, 111, 46, 103, 105, 102]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "UrlToken:425",
        input: &[
            117, 114, 76, 40, 104, 116, 116, 112, 115, 58, 47, 47, 101, 120, 97, 109, 112, 108,
            101, 46, 99, 111, 109, 47, 99, 97, 116, 115, 46, 112, 110, 103, 41,
        ],
        expected: &[NextExpected {
            kind: Kind::Url,
            value: Some(&[
                104, 116, 116, 112, 115, 58, 47, 47, 101, 120, 97, 109, 112, 108, 101, 46, 99, 111,
                109, 47, 99, 97, 116, 115, 46, 112, 110, 103,
            ]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "UrlToken:427",
        input: &[
            117, 82, 108, 40, 119, 104, 97, 116, 45, 97, 46, 99, 114, 97, 122, 121, 94, 85, 82, 76,
            126, 116, 104, 105, 115, 92, 32, 105, 115, 33, 41,
        ],
        expected: &[NextExpected {
            kind: Kind::Url,
            value: Some(&[
                119, 104, 97, 116, 45, 97, 46, 99, 114, 97, 122, 121, 94, 85, 82, 76, 126, 116,
                104, 105, 115, 32, 105, 115, 33,
            ]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "UrlToken:429",
        input: &[117, 82, 76, 40, 49, 50, 51, 35, 116, 101, 115, 116, 41],
        expected: &[NextExpected {
            kind: Kind::Url,
            value: Some(&[49, 50, 51, 35, 116, 101, 115, 116]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "UrlToken:430",
        input: &[
            85, 114, 108, 40, 101, 115, 99, 97, 112, 101, 115, 92, 32, 92, 34, 92, 39, 92, 41, 92,
            40, 41,
        ],
        expected: &[NextExpected {
            kind: Kind::Url,
            value: Some(&[101, 115, 99, 97, 112, 101, 115, 32, 34, 39, 41, 40]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "UrlToken:431",
        input: &[
            85, 114, 76, 40, 32, 32, 32, 119, 104, 105, 116, 101, 115, 112, 97, 99, 101, 32, 32,
            32, 41,
        ],
        expected: &[NextExpected {
            kind: Kind::Url,
            value: Some(&[119, 104, 105, 116, 101, 115, 112, 97, 99, 101]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "UrlToken:432",
        input: &[
            85, 82, 108, 40, 32, 119, 104, 105, 116, 101, 115, 112, 97, 99, 101, 45, 101, 111, 102,
            32,
        ],
        expected: &[NextExpected {
            kind: Kind::Url,
            value: Some(&[119, 104, 105, 116, 101, 115, 112, 97, 99, 101, 45, 101, 111, 102]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "UrlToken:433",
        input: &[85, 82, 76, 40, 101, 111, 102],
        expected: &[NextExpected {
            kind: Kind::Url,
            value: Some(&[101, 111, 102]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "UrlToken:434",
        input: &[
            117, 114, 108, 40, 110, 111, 116, 47, 42, 97, 42, 47, 99, 111, 109, 109, 101, 110, 116,
            41,
        ],
        expected: &[NextExpected {
            kind: Kind::Url,
            value: Some(&[110, 111, 116, 47, 42, 97, 42, 47, 99, 111, 109, 109, 101, 110, 116]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "UrlToken:435",
        input: &[117, 114, 76, 40, 41],
        expected: &[NextExpected {
            kind: Kind::Url,
            value: Some(&[]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "UrlToken:436",
        input: &[117, 82, 108, 40, 119, 104, 105, 116, 101, 32, 115, 112, 97, 99, 101, 41, 44],
        expected: &[
            NextExpected { kind: Kind::BadUrl, value: None, delimiter: None, hash_is_id: false },
            NextExpected { kind: Kind::Comma, value: None, delimiter: None, hash_is_id: false },
        ],
    },
    NextCase {
        label: "UrlToken:437",
        input: &[85, 114, 108, 40, 98, 40, 97, 100, 41, 44],
        expected: &[
            NextExpected { kind: Kind::BadUrl, value: None, delimiter: None, hash_is_id: false },
            NextExpected { kind: Kind::Comma, value: None, delimiter: None, hash_is_id: false },
        ],
    },
    NextCase {
        label: "UrlToken:438",
        input: &[117, 82, 108, 40, 98, 97, 39, 100, 41, 58],
        expected: &[
            NextExpected { kind: Kind::BadUrl, value: None, delimiter: None, hash_is_id: false },
            NextExpected { kind: Kind::Colon, value: None, delimiter: None, hash_is_id: false },
        ],
    },
    NextCase {
        label: "UrlToken:439",
        input: &[117, 114, 76, 40, 98, 34, 97, 100, 41, 58],
        expected: &[
            NextExpected { kind: Kind::BadUrl, value: None, delimiter: None, hash_is_id: false },
            NextExpected { kind: Kind::Colon, value: None, delimiter: None, hash_is_id: false },
        ],
    },
    NextCase {
        label: "UrlToken:440",
        input: &[117, 82, 108, 40, 98, 34, 97, 100, 41, 58],
        expected: &[
            NextExpected { kind: Kind::BadUrl, value: None, delimiter: None, hash_is_id: false },
            NextExpected { kind: Kind::Colon, value: None, delimiter: None, hash_is_id: false },
        ],
    },
    NextCase {
        label: "UrlToken:441",
        input: &[85, 114, 108, 40, 98, 92, 13, 97, 100, 41, 58],
        expected: &[
            NextExpected { kind: Kind::BadUrl, value: None, delimiter: None, hash_is_id: false },
            NextExpected { kind: Kind::Colon, value: None, delimiter: None, hash_is_id: false },
        ],
    },
    NextCase {
        label: "UrlToken:442",
        input: &[117, 114, 108, 40, 98, 92, 10, 97, 100, 41, 58],
        expected: &[
            NextExpected { kind: Kind::BadUrl, value: None, delimiter: None, hash_is_id: false },
            NextExpected { kind: Kind::Colon, value: None, delimiter: None, hash_is_id: false },
        ],
    },
    NextCase {
        label: "UrlToken:443",
        input: &[117, 114, 108, 40, 47, 42, 39, 98, 97, 100, 39, 41, 42, 47],
        expected: &[
            NextExpected { kind: Kind::BadUrl, value: None, delimiter: None, hash_is_id: false },
            NextExpected {
                kind: Kind::Delimiter,
                value: None,
                delimiter: Some(42),
                hash_is_id: false,
            },
            NextExpected {
                kind: Kind::Delimiter,
                value: None,
                delimiter: Some(47),
                hash_is_id: false,
            },
        ],
    },
    NextCase {
        label: "UrlToken:444",
        input: &[117, 114, 108, 40, 98, 97, 39, 100, 92, 92, 41, 41],
        expected: &[
            NextExpected { kind: Kind::BadUrl, value: None, delimiter: None, hash_is_id: false },
            NextExpected { kind: Kind::RParen, value: None, delimiter: None, hash_is_id: false },
        ],
    },
    NextCase {
        label: "StringToken:448",
        input: &[39, 116, 101, 120, 116, 39],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[116, 101, 120, 116]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:449",
        input: &[34, 116, 101, 120, 116, 34],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[116, 101, 120, 116]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:450",
        input: &[39, 116, 101, 115, 116, 105, 110, 103, 44, 32, 49, 50, 51, 33, 39],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[116, 101, 115, 116, 105, 110, 103, 44, 32, 49, 50, 51, 33]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:451",
        input: &[39, 101, 115, 92, 39, 99, 97, 92, 34, 112, 101, 39],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[101, 115, 39, 99, 97, 34, 112, 101]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:452",
        input: &[39, 34, 113, 117, 111, 116, 101, 115, 34, 39],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[34, 113, 117, 111, 116, 101, 115, 34]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:453",
        input: &[34, 39, 113, 117, 111, 116, 101, 115, 39, 34],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[39, 113, 117, 111, 116, 101, 115, 39]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:454",
        input: &[34, 109, 105, 115, 109, 97, 116, 99, 104, 39],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[109, 105, 115, 109, 97, 116, 99, 104, 39]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:455",
        input: &[39, 116, 101, 120, 116, 5, 9, 11, 39],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[116, 101, 120, 116, 5, 9, 11]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:456",
        input: &[34, 101, 110, 100, 32, 111, 110, 32, 101, 111, 102],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[101, 110, 100, 32, 111, 110, 32, 101, 111, 102]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:457",
        input: &[39, 101, 115, 99, 97, 92, 10, 112, 101, 100, 39],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[101, 115, 99, 97, 112, 101, 100]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:458",
        input: &[34, 101, 115, 99, 92, 12, 97, 112, 101, 100, 34],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[101, 115, 99, 97, 112, 101, 100]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:459",
        input: &[39, 110, 101, 119, 92, 13, 108, 105, 110, 101, 39],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[110, 101, 119, 108, 105, 110, 101]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:460",
        input: &[34, 110, 101, 119, 92, 13, 10, 108, 105, 110, 101, 34],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[110, 101, 119, 108, 105, 110, 101]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:461",
        input: &[39, 98, 97, 100, 10, 115, 116, 114, 105, 110, 103],
        expected: &[
            NextExpected { kind: Kind::BadString, value: None, delimiter: None, hash_is_id: false },
            NextExpected {
                kind: Kind::Whitespace,
                value: None,
                delimiter: None,
                hash_is_id: false,
            },
            NextExpected {
                kind: Kind::Ident,
                value: Some(&[115, 116, 114, 105, 110, 103]),
                delimiter: None,
                hash_is_id: false,
            },
        ],
    },
    NextCase {
        label: "StringToken:462",
        input: &[39, 98, 97, 100, 13, 115, 116, 114, 105, 110, 103],
        expected: &[
            NextExpected { kind: Kind::BadString, value: None, delimiter: None, hash_is_id: false },
            NextExpected {
                kind: Kind::Whitespace,
                value: None,
                delimiter: None,
                hash_is_id: false,
            },
            NextExpected {
                kind: Kind::Ident,
                value: Some(&[115, 116, 114, 105, 110, 103]),
                delimiter: None,
                hash_is_id: false,
            },
        ],
    },
    NextCase {
        label: "StringToken:463",
        input: &[39, 98, 97, 100, 13, 10, 115, 116, 114, 105, 110, 103],
        expected: &[
            NextExpected { kind: Kind::BadString, value: None, delimiter: None, hash_is_id: false },
            NextExpected {
                kind: Kind::Whitespace,
                value: None,
                delimiter: None,
                hash_is_id: false,
            },
            NextExpected {
                kind: Kind::Ident,
                value: Some(&[115, 116, 114, 105, 110, 103]),
                delimiter: None,
                hash_is_id: false,
            },
        ],
    },
    NextCase {
        label: "StringToken:464",
        input: &[39, 98, 97, 100, 12, 115, 116, 114, 105, 110, 103],
        expected: &[
            NextExpected { kind: Kind::BadString, value: None, delimiter: None, hash_is_id: false },
            NextExpected {
                kind: Kind::Whitespace,
                value: None,
                delimiter: None,
                hash_is_id: false,
            },
            NextExpected {
                kind: Kind::Ident,
                value: Some(&[115, 116, 114, 105, 110, 103]),
                delimiter: None,
                hash_is_id: false,
            },
        ],
    },
    NextCase {
        label: "StringToken:465",
        input: &[39, 0, 39],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[65533]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:467",
        input: &[39, 104, 101, 108, 0, 108, 111, 39],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[104, 101, 108, 65533, 108, 111]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "StringToken:469",
        input: &[39, 104, 92, 54, 53, 108, 0, 108, 111, 39],
        expected: &[NextExpected {
            kind: Kind::String,
            value: Some(&[104, 101, 108, 65533, 108, 111]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "HashToken:474",
        input: &[35, 105, 100, 45, 115, 101, 108, 101, 99, 116, 111, 114],
        expected: &[NextExpected {
            kind: Kind::Hash,
            value: Some(&[105, 100, 45, 115, 101, 108, 101, 99, 116, 111, 114]),
            delimiter: None,
            hash_is_id: true,
        }],
    },
    NextCase {
        label: "HashToken:475",
        input: &[35, 70, 70, 55, 55, 48, 48],
        expected: &[NextExpected {
            kind: Kind::Hash,
            value: Some(&[70, 70, 55, 55, 48, 48]),
            delimiter: None,
            hash_is_id: true,
        }],
    },
    NextCase {
        label: "HashToken:476",
        input: &[35, 51, 51, 55, 55, 70, 70],
        expected: &[NextExpected {
            kind: Kind::Hash,
            value: Some(&[51, 51, 55, 55, 70, 70]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "HashToken:477",
        input: &[35, 92, 32],
        expected: &[NextExpected {
            kind: Kind::Hash,
            value: Some(&[32]),
            delimiter: None,
            hash_is_id: true,
        }],
    },
    NextCase {
        label: "HashToken:478",
        input: &[35, 0],
        expected: &[NextExpected {
            kind: Kind::Hash,
            value: Some(&[65533]),
            delimiter: None,
            hash_is_id: true,
        }],
    },
    NextCase {
        label: "HashToken:480",
        input: &[35, 45, 0],
        expected: &[NextExpected {
            kind: Kind::Hash,
            value: Some(&[45, 65533]),
            delimiter: None,
            hash_is_id: true,
        }],
    },
    NextCase {
        label: "HashToken:482",
        input: &[35, 32],
        expected: &[
            NextExpected {
                kind: Kind::Delimiter,
                value: None,
                delimiter: Some(35),
                hash_is_id: false,
            },
            NextExpected {
                kind: Kind::Whitespace,
                value: None,
                delimiter: None,
                hash_is_id: false,
            },
        ],
    },
    NextCase {
        label: "HashToken:483",
        input: &[35, 92, 10],
        expected: &[
            NextExpected {
                kind: Kind::Delimiter,
                value: None,
                delimiter: Some(35),
                hash_is_id: false,
            },
            NextExpected {
                kind: Kind::Delimiter,
                value: None,
                delimiter: Some(92),
                hash_is_id: false,
            },
            NextExpected {
                kind: Kind::Whitespace,
                value: None,
                delimiter: None,
                hash_is_id: false,
            },
        ],
    },
    NextCase {
        label: "HashToken:484",
        input: &[35, 92, 13, 10],
        expected: &[
            NextExpected {
                kind: Kind::Delimiter,
                value: None,
                delimiter: Some(35),
                hash_is_id: false,
            },
            NextExpected {
                kind: Kind::Delimiter,
                value: None,
                delimiter: Some(92),
                hash_is_id: false,
            },
            NextExpected {
                kind: Kind::Whitespace,
                value: None,
                delimiter: None,
                hash_is_id: false,
            },
        ],
    },
    NextCase {
        label: "HashToken:485",
        input: &[35, 33],
        expected: &[
            NextExpected {
                kind: Kind::Delimiter,
                value: None,
                delimiter: Some(35),
                hash_is_id: false,
            },
            NextExpected {
                kind: Kind::Delimiter,
                value: None,
                delimiter: Some(33),
                hash_is_id: false,
            },
        ],
    },
    NextCase {
        label: "CommentToken:585",
        input: &[47, 42, 99, 111, 109, 109, 101, 110, 116, 42, 47, 97],
        expected: &[NextExpected {
            kind: Kind::Ident,
            value: Some(&[97]),
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "CommentToken:586",
        input: &[47, 42, 42, 92, 50, 102, 42, 42, 47, 47],
        expected: &[NextExpected {
            kind: Kind::Delimiter,
            value: None,
            delimiter: Some(47),
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "CommentToken:587",
        input: &[47, 42, 42, 121, 42, 97, 42, 121, 42, 42, 47, 32],
        expected: &[NextExpected {
            kind: Kind::Whitespace,
            value: None,
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "CommentToken:588",
        input: &[44, 47, 42, 32, 10, 32, 58, 41, 32, 10, 32, 42, 47, 41],
        expected: &[
            NextExpected { kind: Kind::Comma, value: None, delimiter: None, hash_is_id: false },
            NextExpected { kind: Kind::RParen, value: None, delimiter: None, hash_is_id: false },
        ],
    },
    NextCase {
        label: "CommentToken:589",
        input: &[58, 47, 42, 47, 42, 47],
        expected: &[NextExpected {
            kind: Kind::Colon,
            value: None,
            delimiter: None,
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "CommentToken:590",
        input: &[47, 42, 42, 47, 42],
        expected: &[NextExpected {
            kind: Kind::Delimiter,
            value: None,
            delimiter: Some(42),
            hash_is_id: false,
        }],
    },
    NextCase {
        label: "CommentToken:591",
        input: &[59, 47, 42, 42, 42, 42, 42, 42],
        expected: &[NextExpected {
            kind: Kind::Semicolon,
            value: None,
            delimiter: None,
            hash_is_id: false,
        }],
    },
];
