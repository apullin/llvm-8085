#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

// Output base address (i8085-trace simulator memory)
const OUTPUT_BASE: usize = 0x0200;

fn output_byte(offset: usize, b: u8) {
    unsafe {
        core::ptr::write_volatile((OUTPUT_BASE + offset) as *mut u8, b);
    }
}

// CRT0 calls main() and then executes HLT, so we just return.
#[no_mangle]
pub extern "C" fn main() -> i16 {
    // Test 1: Simple arithmetic
    let x: i16 = 40;
    let y: i16 = 2;
    let sum = x + y; // 42
    output_byte(0, sum as u8); // expect 0x2A

    // Test 2: u8 arithmetic
    let a: u8 = 100;
    let b: u8 = 55;
    output_byte(1, a + b); // expect 0x9B (155)

    // Test 3: Boolean/control flow
    if sum == 42 {
        output_byte(2, 0x01); // pass
    } else {
        output_byte(2, 0x00); // fail
    }

    // Test 4: Loop (sum 1..10 = 55)
    let mut total: u8 = 0;
    let mut i: u8 = 1;
    while i <= 10 {
        total += i;
        i += 1;
    }
    output_byte(3, total); // expect 0x37 (55)

    0
}
