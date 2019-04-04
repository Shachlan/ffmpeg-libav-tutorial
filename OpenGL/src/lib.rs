extern crate libc;
extern crate glfw;

use libc::c_float;
use libc::c_int;
use libc::c_char;
use std;

#[repr(C)]
pub struct TextureInfo
{
    buffer: *const c_char,
    width: c_int,
    height: c_int
}

struct ProgramInfo {
    program: isize,
    texture_count: isize,
    textures: [isize; 3],
    position_buffers: isize,
    texture_buffer: isize
}

static mut invert_program: ProgramInfo = ProgramInfo {
        program: -1, texture_count:-1, textures:[-1, -1, -1], position_buffers:-1, texture_buffer:-1
    };
static mut blend_program: ProgramInfo = ProgramInfo {
        program: -1, texture_count:-1, textures:[-1, -1, -1], position_buffers:-1, texture_buffer:-1
    };

static mut window: *const glfw::Window = std::ptr::null();

#[no_mangle]
pub extern "C" fn setupOpenGL(width: c_int, height: c_int, blend_ratio: c_float) {
    println!("setup");
    let mut glfw = glfw::init(glfw::FAIL_ON_ERRORS).unwrap();
    glfw.window_hint(glfw::WindowHint::Visible(false));
    let (mut internalWindow, events) = glfw.create_window(width as u32, height as u32, "", glfw::WindowMode::Windowed)
        .expect("Failed to create GLFW window.");
        unsafe {
    window = &internalWindow;
        }
}

#[no_mangle]
pub extern "C" fn invertFrame(tex: TextureInfo) {
    println!("invertFrame");
}

#[no_mangle]
pub extern "C" fn blendFrames(target: TextureInfo, tex1: TextureInfo, tex2: TextureInfo) {
    println!("blendFrames");
}

#[no_mangle]
pub extern "C" fn tearDownOpenGL() {
    println!("tearDownOpenGL");
}
