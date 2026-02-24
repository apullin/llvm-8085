//! Utilities for serializing and deserializing strings.

use core::fmt;

#[derive(Debug)]
/// A fragment of an escaped string
pub enum EscapedStringFragment<'a> {
    /// A series of characters which weren't escaped in the input.
    NotEscaped(&'a str),
    /// A character which was escaped in the input.
    Escaped(char),
}

#[derive(Debug)]
/// Errors occuring while unescaping strings.
pub enum StringUnescapeError {
    /// Failed to unescape a character due to an invalid escape sequence.
    InvalidEscapeSequence,
}

impl fmt::Display for StringUnescapeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            StringUnescapeError::InvalidEscapeSequence => write!(
                f,
                "Failed to unescape a character due to an invalid escape sequence."
            ),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for StringUnescapeError {}

fn unescape_next_fragment(
    escaped_string: &str,
) -> Result<(EscapedStringFragment<'_>, &str), StringUnescapeError> {
    let bytes = escaped_string.as_bytes();
    if bytes.is_empty() {
        return Err(StringUnescapeError::InvalidEscapeSequence);
    }
    Ok(if bytes[0] == b'\\' {
        if bytes.len() < 2 {
            return Err(StringUnescapeError::InvalidEscapeSequence);
        }
        let unescaped_char = match bytes[1] {
            b'"' => '"',
            b'\\' => '\\',
            b'/' => '/',
            b'b' => '\x08',
            b'f' => '\x0C',
            b'n' => '\n',
            b'r' => '\r',
            b't' => '\t',
            _ => return Err(StringUnescapeError::InvalidEscapeSequence),
        };
        // SAFETY: slicing at byte offset 2 is safe since we checked len >= 2
        // and escape sequences are all single ASCII bytes
        let rest = unsafe { core::str::from_utf8_unchecked(&bytes[2..]) };
        (
            EscapedStringFragment::Escaped(unescaped_char),
            rest,
        )
    } else {
        // Find next backslash by scanning bytes
        let mut pos = 0;
        while pos < bytes.len() && bytes[pos] != b'\\' {
            pos += 1;
        }
        // SAFETY: the input is valid UTF-8 and we split at ASCII byte boundaries
        let fragment = unsafe { core::str::from_utf8_unchecked(&bytes[..pos]) };
        let rest = unsafe { core::str::from_utf8_unchecked(&bytes[pos..]) };
        (EscapedStringFragment::NotEscaped(fragment), rest)
    })
}

/// A borrowed escaped string. `EscapedStr` can be used to borrow an escaped string from the input,
/// even when deserialized using `from_str_escaped` or `from_slice_escaped`.
///
/// ```
///     #[derive(serde::Deserialize)]
///     struct Event<'a> {
///         name: heapless::String<16>,
///         #[serde(borrow)]
///         description: serde_json_core::str::EscapedStr<'a>,
///     }
///     
///     serde_json_core::de::from_str_escaped::<Event<'_>>(
///         r#"{ "name": "Party\u0021", "description": "I'm throwing a party! Hopefully the \u2600 shines!" }"#,
///         &mut [0; 8],
///     )
///     .unwrap();
/// ```
#[derive(Debug, Clone, Copy, PartialEq, Eq, serde::Serialize, serde::Deserialize)]
#[serde(rename = "__serde_json_core_escaped_string__")]
pub struct EscapedStr<'a>(pub &'a str);

impl<'a> EscapedStr<'a> {
    pub(crate) const NAME: &'static str = "__serde_json_core_escaped_string__";

    /// Returns an iterator over the `EscapedStringFragment`s of an escaped string.
    pub fn fragments(&self) -> EscapedStringFragmentIter<'a> {
        EscapedStringFragmentIter(self.0)
    }
}

/// An iterator over the `EscapedStringFragment`s of an escaped string.
pub struct EscapedStringFragmentIter<'a>(&'a str);

impl<'a> EscapedStringFragmentIter<'a> {
    /// Views the underlying data as a subslice of the original data.
    pub fn as_str(&self) -> EscapedStr<'a> {
        EscapedStr(self.0)
    }
}

impl<'a> Iterator for EscapedStringFragmentIter<'a> {
    type Item = Result<EscapedStringFragment<'a>, StringUnescapeError>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.0.is_empty() {
            return None;
        }

        Some(unescape_next_fragment(self.0).map(|(fragment, rest)| {
            self.0 = rest;

            fragment
        }))
    }
}
