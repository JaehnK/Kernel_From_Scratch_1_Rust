#![no_std]
#![no_main]

use core::panic::PanicInfo;

const VGA_BUFFER: *mut u8 = 0xb8000 as *mut u8;

#[no_mangle]
pub extern "C" fn kernel_main() {
    unsafe {
        *VGA_BUFFER = b'4';
        *VGA_BUFFER.add(1) = 0x0f;
        *VGA_BUFFER.add(2) = b'2';
        *VGA_BUFFER.add(3) = 0x0f;
    }
    loop {}
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
