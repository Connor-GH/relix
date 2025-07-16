use crate::printing::*;
use alloc::vec::Vec;
use anstyle_parse::{DefaultCharAccumulator, Params, Parser, Perform};
use core::ffi::c_char;

unsafe extern "C" {
    fn ansi_change_color(bold: bool, color: u32, c: c_char, fg: bool);
    fn ansi_set_cursor_location_x(x: u16);
    fn ansi_set_cursor_location_y(y: u16);
    fn ansi_set_cursor_location_up(by: u16);
    fn ansi_set_cursor_location(x: u16, y: u16);
    safe fn ansi_4bit_to_hex_color(color: u16, is_bg: bool) -> u32;
    fn ansi_erase_in_front_of_cursor();
}
/// A type implementing Perform that just logs actions
struct Log;

impl Perform for Log {
    fn print(&mut self, c: char) {
        debugln!("[print] {:?}", c);
    }

    fn execute(&mut self, byte: u8) {
        debugln!("[execute] {:02x}", byte);
    }

    fn hook(&mut self, params: &Params, intermediates: &[u8], ignore: bool, c: u8) {
        debugln!(
            "[hook] params={:?}, intermediates={:?}, ignore={:?}, char={:?}",
            params,
            intermediates,
            ignore,
            c
        );
    }

    fn put(&mut self, byte: u8) {
        debugln!("[put] {:02x}", byte);
    }

    fn unhook(&mut self) {
        debugln!("[unhook]");
    }

    fn osc_dispatch(&mut self, params: &[&[u8]], bell_terminated: bool) {
        debugln!(
            "[osc_dispatch] params={:?} bell_terminated={}",
            params,
            bell_terminated
        );
    }

    fn csi_dispatch(&mut self, params: &Params, intermediates: &[u8], ignore: bool, c: u8) {
        debugln!(
            "[hook] params={:?}, intermediates={:?}, ignore={:?}, char={:?}",
            params,
            intermediates,
            ignore,
            c.as_ascii().unwrap()
        );
        let param_vec: Vec<&u16> = params.iter().flatten().collect();
        match c {
            // color parsing.
            b'm' => {
                let mut bold = false;
                let mut color: u32 = 0;
                let mut changing_fg = false;
                for param in param_vec {
                    match *param {
                        0 => {
                            color = ansi_4bit_to_hex_color(0, false);
                        }
                        1 => {
                            bold = true;
                        }
                        n if n >= 30 && n <= 37 => {
                            color = ansi_4bit_to_hex_color(n + (bold as u16 * 60), false);
                            changing_fg = true;
                        }
                        n if n >= 40 && n <= 47 => {
                            color = ansi_4bit_to_hex_color(n + (bold as u16 * 60), true);
                            changing_fg = false;
                        }
                        _ => {}
                    }
                }
                unsafe { ansi_change_color(bold, color, c as c_char, changing_fg) };
                return;
            }
            // Cursor position.
            b'H' => {
                if param_vec.len() != 2 {
                    return;
                }
                // Locations are '1'-based.
                unsafe {
                    ansi_set_cursor_location(*param_vec[0] - 1, *param_vec[1] - 1);
                }
                return;
            }
            b'A' => {
                let n: u16 = **param_vec.get(0).unwrap_or(&&1);
                unsafe {
                    ansi_set_cursor_location_up(n);
                }
                return;
            }
            b'K' => {
                let n: u16 = **param_vec.get(0).unwrap_or(&&0u16);
                match n {
                    0 => unsafe {
                        ansi_erase_in_front_of_cursor();
                    },
                    1 => {}
                    2 => {}
                    _ => {}
                }
                return;
            }
            _ => {}
        }
    }

    fn esc_dispatch(&mut self, intermediates: &[u8], ignore: bool, byte: u8) {
        debugln!(
            "[esc_dispatch] intermediates={:?}, ignore={:?}, byte={:02x}",
            intermediates,
            ignore,
            byte
        );
    }
}
use core::ffi::CStr;
#[unsafe(no_mangle)]
pub extern "C" fn let_rust_handle_it(fmt: *const c_char) -> usize {
    let fmt = unsafe { CStr::from_ptr(fmt) }.to_str().unwrap();
    let index = fmt
        .find(|arg0: char| char::is_ascii_alphabetic(&arg0))
        .unwrap_or(0);
    parse_ansi(&fmt[0..index + 1]);
    index
}
fn parse_ansi(code: &str) {
    let mut statemachine = Parser::<DefaultCharAccumulator>::new();
    let mut performer = Log;
    for c in code.as_bytes() {
        statemachine.advance(&mut performer, *c);
    }
}
