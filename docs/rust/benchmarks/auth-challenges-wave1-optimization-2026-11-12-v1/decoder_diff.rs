#[path = "D:/Rustium/net/http/auth_challenge_core.rs"]
mod core;
extern "C" { fn auth_reference_decode(dest: *mut u8, src: *const u8, len: usize) -> usize; }
fn check(input: &[u8]) {
    let mut output = vec![0u8; input.len() / 4 * 3 + 2];
    // The frozen decoder writes at most the capacity above and reads input.len() bytes.
    let len = unsafe { auth_reference_decode(output.as_mut_ptr(), input.as_ptr(), input.len()) };
    let mut candidate = vec![0u8; input.len() / 4 * 3 + 2];
    let actual = core::decode_strict_into(input, &mut candidate).map(|n| { candidate.truncate(n); candidate });
    if len == usize::MAX { assert_eq!(actual, None, "input={input:?}"); }
    else { output.truncate(len); assert_eq!(actual, Some(output), "input={input:?}"); }
}
fn main() {
    let alphabet = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/= \t\r\n\0\xff";
    let mut state = 0x123456789abcdefu64;
    for i in 0..1_000_000 {
        let mut input = Vec::with_capacity(48);
        state ^= state << 13; state ^= state >> 7; state ^= state << 17;
        let len = (state % 49) as usize;
        for _ in 0..len {
            state ^= state << 13; state ^= state >> 7; state ^= state << 17;
            input.push(if i % 3 == 0 { state as u8 } else { alphabet[state as usize % alphabet.len()] });
        }
        check(&input);
    }
    println!("1000000 decoder differential cases passed");
}

