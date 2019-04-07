extern crate libc;
extern crate glfw;
use self::glfw::{Context, Key, Action};

extern crate gl;
use self::gl::types::*;

mod shader;
use shader::Shader;

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
    position_buffers: isize,
    texture_buffer: isize
}

const position: [f32; 12] = [ -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0];

const textureCoords: [f32; 12] = [ 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0];

static mut invert_program: ProgramInfo = ProgramInfo {
        program: -1, position_buffers:-1, texture_buffer:-1
    };
static mut blend_program: ProgramInfo = ProgramInfo {
        program: -1, position_buffers:-1, texture_buffer:-1
    };

static mut window: *const glfw::Window = std::ptr::null();
static mut texture1: u32 = 0;
static mut texture2: u32 = 0;

fn build_program(vertex_shader_filename: &str, fragment_shader_filename: &str) {

}

fn setupBlendingProgram() {
    let program = Shader::new("passthrough.vsh", "blend.fsh");
}

fn setupInvertProgram() {
    //glUniform1i(glGetUniformLocation(program, textureName), textureNum);
}

fn tex_setup() -> u32
{
    unsafe {
        let mut texture = 0;
        gl::ActiveTexture(texture);
        gl::GenTextures(1, &mut texture);
        gl::BindTexture(gl::TEXTURE_2D, texture);

        gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_S, gl::REPEAT as i32); 
        gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_T, gl::REPEAT as i32);
        gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::LINEAR as i32);
        gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::LINEAR as i32);

        texture
    }
}

#[no_mangle]
pub extern "C" fn setupOpenGL(width: c_int, height: c_int, blend_ratio: c_float) {
    println!("setup");
    let mut glfw = glfw::init(glfw::FAIL_ON_ERRORS).unwrap();
    // glfw.window_hint(glfw::WindowHint::ContextVersion(3, 3));
    // glfw.window_hint(glfw::WindowHint::OpenGlProfile(glfw::OpenGlProfileHint::Core));
    glfw.window_hint(glfw::WindowHint::Visible(false));
    let (mut internalWindow, events) = glfw.create_window(width as u32, height as u32, "", glfw::WindowMode::Windowed)
        .expect("Failed to create GLFW window.");
    unsafe {
        window = &internalWindow;
        internalWindow.make_current();
        gl::load_with(|symbol| internalWindow.get_proc_address(symbol) as *const _);
        gl::Viewport(0, 0, width, height); 
        texture1 = tex_setup();
        texture2 = tex_setup();
    }

    setupBlendingProgram();
    setupInvertProgram();
}

#[no_mangle]
pub extern "C" fn invertFrame(tex: TextureInfo) {
    //println!("invertFrame");
}

#[no_mangle]
pub extern "C" fn blendFrames(target: TextureInfo, tex1: TextureInfo, tex2: TextureInfo) {
//println!("blendFrames");
}

#[no_mangle]
pub extern "C" fn tearDownOpenGL() {
    unsafe {
        gl::DeleteTextures(1, &texture1);
        gl::DeleteTextures(1, &texture2);
    }
    //println!("tearDownOpenGL");
}
