"""Deterministic comment-stripped nonblank C/C++ physical code lines.

Braces, declarations, preprocessor directives and literal-content lines count.
This is NCLOC weighting, not a statement counter or complexity estimate.
"""
import re

_TOKENS = re.compile(
    r'(?:u8|u|U|L)?R"(?P<delimiter>[^\s\\()]{0,16})\([\s\S]*?\)(?P=delimiter)"'
    r'|"(?:\\[\s\S]|[^"\\])*"'
    r"|'(?:\\[\s\S]|[^'\\\r\n])*'"
    r'|//[^\r\n]*|/\*[\s\S]*?\*/'
)


def strip_comments(text):
    def replace(match):
        token = match.group(0)
        if token.startswith(('//', '/*')):
            return ''.join(c if c in '\r\n' else ' ' for c in token)
        return token
    return _TOKENS.sub(replace, text)


def code_line_numbers(text, language='cpp'):
    if language not in {'cpp', 'c', 'objc'}:
        raise ValueError('This lexer is qualified only for C/C++/Objective-C source')
    return [i for i, line in enumerate(strip_comments(text).splitlines(), 1) if line.strip()]


def count_code_lines(text, language='cpp'):
    return len(code_line_numbers(text, language))


def self_check():
    cases = [
        ('// only\n/* only */\n', 0),
        ('int x; // suffix\n', 1),
        ('int /* middle */ x;\n', 1),
        ('/* open\nclose */ int x;\n', 1),
        ('const char* s = "http://host/*literal*/";\n', 1),
        ("const char q = '\\''; // suffix\n", 1),
        ('const auto s = R"TAG(//literal\n/*literal*/\n)TAG";\n', 3),
        ('#define F(x) \\\n  (x)\n', 2),
        ('int f() {\n  return 1;\n}\n', 3),
        ('/* comment\n\n*/\n\t \n', 0),
    ]
    for text, expected in cases:
        assert count_code_lines(text) == expected, (text, code_line_numbers(text), expected)
    return len(cases)


if __name__ == '__main__':
    print('NCLOC lexer checks:', self_check())
