#![no_std]
#![no_main]

use core::panic::PanicInfo;
use serde::{Serialize, Deserialize};

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

const OUTPUT_BASE: usize = 0x0200;

fn output_byte(offset: usize, b: u8) {
    unsafe {
        core::ptr::write_volatile((OUTPUT_BASE + offset) as *mut u8, b);
    }
}

#[derive(Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
struct Point {
    x: i16,
    y: i16,
    z: i16,
}

#[no_mangle]
pub extern "C" fn main() -> i16 {
    let p = Point { x: 42, y: -7, z: 300 };

    // Serialize to JSON
    let mut buf = [0u8; 64];
    let len = match serde_json_core::to_slice(&p, &mut buf) {
        Ok(n) => n,
        Err(_) => {
            output_byte(0, 0xFF); // error marker
            return 1;
        }
    };

    // Output JSON string length
    output_byte(0, len as u8);

    // Output the JSON bytes so we can see them in the dump
    for i in 0..len {
        output_byte(1 + i, buf[i]);
    }

    // Deserialize back from JSON
    let p2: Point = match serde_json_core::from_slice(&buf[..len]) {
        Ok((point, _)) => point,
        Err(_) => {
            output_byte(1 + len, 0xFF); // error marker
            return 2;
        }
    };

    // Output deserialized fields (little-endian)
    let off = 1 + len;
    output_byte(off,     p2.x as u8);          // x low = 42
    output_byte(off + 1, (p2.x >> 8) as u8);   // x high = 0
    output_byte(off + 2, p2.y as u8);          // y low = 0xF9 (-7)
    output_byte(off + 3, (p2.y >> 8) as u8);   // y high = 0xFF
    output_byte(off + 4, p2.z as u8);          // z low = 0x2C (300)
    output_byte(off + 5, (p2.z >> 8) as u8);   // z high = 0x01

    // Sentinel
    output_byte(off + 6, 0xBE);
    output_byte(off + 7, 0xEF);

    0
}
