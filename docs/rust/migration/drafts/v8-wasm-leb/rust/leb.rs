//! Safe semantic candidate for V8's WebAssembly LEB helper and decoder paths.
//! This is a proposal module: no production exports, ABI, or GN wiring.

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Width {
    U32,
    I32,
    U64,
    I64,
    I33,
}

impl Width {
    pub const fn bits(self) -> u32 {
        match self {
            Self::U32 | Self::I32 => 32,
            Self::U64 | Self::I64 => 64,
            Self::I33 => 33,
        }
    }
    pub const fn signed(self) -> bool {
        !matches!(self, Self::U32 | Self::U64)
    }
    pub const fn max_bytes(self) -> usize {
        ((self.bits() + 6) / 7) as usize
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ValidationMode {
    Full,
    AssumeValid,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Decoded {
    pub value: i128,
    pub bytes: usize,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct DecodeError {
    pub offset: usize,
    pub kind: DecodeErrorKind,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum DecodeErrorKind {
    ReachedEnd,
    LengthOverflow,
    ExtraBits,
}

impl DecodeError {
    /// Mirrors V8's message forms; caller supplies the original decoder name.
    pub fn native_message(&self, name: &str) -> String {
        match self.kind {
            DecodeErrorKind::ReachedEnd => format!("reached end while decoding {name}"),
            DecodeErrorKind::LengthOverflow => format!("length overflow while decoding {name}"),
            DecodeErrorKind::ExtraBits => "extra bits in varint".to_owned(),
        }
    }
}

fn valid_final(byte: u8, width: Width) -> bool {
    let bits = width.bits();
    let used = bits - (width.max_bytes() as u32 - 1) * 7;
    if !width.signed() {
        return (byte as u32) < (1u32 << used);
    }
    let invalid_mask = 0x100u32 | (0x7fu32 & (0xffu32 << used));
    let b = byte as u32;
    ((b ^ (b << 1)) & invalid_mask) == 0
}

/// Decode one value from `input`. Offsets are relative to the supplied slice;
/// an enclosing decoder adds its base offset when adapting native diagnostics.
/// Both modes remain memory-safe. `AssumeValid` documents the native fast-path
/// precondition; malformed input is still returned as an error rather than UB.
pub fn decode(input: &[u8], width: Width, _mode: ValidationMode) -> Result<Decoded, DecodeError> {
    let max = width.max_bytes();
    let mut raw = 0u128;
    for i in 0..max {
        let Some(&byte) = input.get(i) else {
            return Err(DecodeError { offset: input.len(), kind: DecodeErrorKind::ReachedEnd });
        };
        let shift = (i * 7) as u32;
        raw |= ((byte & 0x7f) as u128) << shift;
        let last = i + 1 == max;
        if !last && byte & 0x80 != 0 {
            continue;
        }
        if last && byte & 0x80 != 0 {
            return Err(DecodeError { offset: i, kind: DecodeErrorKind::LengthOverflow });
        }
        if last && !valid_final(byte, width) {
            return Err(DecodeError { offset: i, kind: DecodeErrorKind::ExtraBits });
        }
        let used = ((i + 1) * 7).min(width.bits()) as u32;
        if width.signed() && byte & 0x40 != 0 && used < 128 {
            raw |= (!0u128) << used;
        }
        let value = if width.signed() { raw as i128 } else { raw as i128 };
        return Ok(Decoded { value, bytes: i + 1 });
    }
    unreachable!("every bounded LEB either terminates or errors")
}

pub fn encode_unsigned(mut value: u64, width: Width) -> Result<Vec<u8>, &'static str> {
    if width.signed() {
        return Err("signed width passed to unsigned encoder");
    }
    if width.bits() < 64 && value >= (1u64 << width.bits()) {
        return Err("value out of range");
    }
    let mut out = Vec::with_capacity(width.max_bytes());
    loop {
        let mut byte = (value & 0x7f) as u8;
        value >>= 7;
        if value != 0 {
            byte |= 0x80;
        }
        out.push(byte);
        if value == 0 {
            return Ok(out);
        }
    }
}

pub fn encode_signed(mut value: i64, width: Width) -> Result<Vec<u8>, &'static str> {
    if !width.signed() {
        return Err("unsigned width passed to signed encoder");
    }
    let bits = width.bits();
    let min = -(1i128 << (bits - 1));
    let max = (1i128 << (bits - 1)) - 1;
    if (value as i128) < min || (value as i128) > max {
        return Err("value out of range");
    }
    let mut out = Vec::with_capacity(width.max_bytes());
    loop {
        let payload = (value as u8) & 0x7f;
        value >>= 7;
        let done = (value == 0 && payload & 0x40 == 0) || (value == -1 && payload & 0x40 != 0);
        out.push(if done { payload } else { payload | 0x80 });
        if done {
            return Ok(out);
        }
    }
}

pub fn encoded_len_unsigned(value: u64) -> usize {
    let mut v = value;
    let mut n = 1;
    while v >= 0x80 {
        n += 1;
        v >>= 7;
    }
    n
}

pub fn encoded_len_signed(value: i64) -> usize {
    let mut v = value;
    let mut n = 1;
    if v >= 0 {
        while v >= 0x40 {
            n += 1;
            v >>= 7;
        }
    } else {
        while (v >> 6) != -1 {
            n += 1;
            v >>= 7;
        }
    }
    n
}

#[cfg(test)]
mod tests;
