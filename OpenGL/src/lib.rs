extern crate libc;
extern crate glfw;
use self::glfw::{Context};

extern crate gl;
use self::gl::types::*;

mod shader;
use shader::Shader;

use libc::c_float;
use libc::c_int;
use libc::c_char;
use std::ffi::{CString, CStr};
use std;
use std::mem;
use std::str;

#[repr(C)]
pub struct TextureInfo
{
    buffer: *const c_char,
    width: c_int,
    height: c_int
}

struct ProgramInfo {
    program: Shader,
    position_buffer: u32,
    texture_buffer: u32
}

const POSITION: [f32; 12] = [ -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0];

const TEXTURE_COORDS: [f32; 12] = [ 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0];

static mut INVERT_PROGRAM: ProgramInfo = ProgramInfo {
        program: Shader { ID: 0 }, position_buffer:0, texture_buffer:0
    };
static mut BLEND_PROGRAM: ProgramInfo = ProgramInfo {
        program: Shader { ID: 0 }, position_buffer:0, texture_buffer:0
    };

static mut WINDOW: *const glfw::Window = std::ptr::null();
static mut TEXTURE1: u32 = 0;
static mut TEXTURE2: u32 = 0;

fn to_string2(source: &str) -> CString {
    unsafe {
        CString::new(source).unwrap()
    }
}

fn to_string(source: &str) -> &CStr {
    unsafe {
    CStr::from_ptr(CString::new(source).unwrap().as_ptr())
    }
}

fn setup_texture_buffer(program: u32) -> u32 {
    let mut texture_buffer = 0;
    unsafe {
    gl::GenBuffers(1, &mut texture_buffer);
    gl::BindBuffer(gl::ARRAY_BUFFER, texture_buffer);
    gl::BufferData(gl::ARRAY_BUFFER, 
        (TEXTURE_COORDS.len() * mem::size_of::<GLfloat>()) as GLsizeiptr, 
        mem::transmute(&TEXTURE_COORDS[0]),
        gl::STATIC_DRAW);

    let loc = gl::GetAttribLocation(program, CString::new("texCoord").unwrap().as_ptr()) as GLuint;
    gl::EnableVertexAttribArray(loc);
    gl::VertexAttribPointer(loc, 2, gl::FLOAT, gl::FALSE, 0, 0 as *const GLvoid);
    }

    texture_buffer
}

fn setup_position_buffer(program: u32) -> u32 {
    let mut position_buffer = 0;
    unsafe {
    gl::GenBuffers(1, &mut position_buffer);
    gl::BindBuffer(gl::ARRAY_BUFFER, position_buffer);
    gl::BufferData(gl::ARRAY_BUFFER, 
        (POSITION.len() * mem::size_of::<GLfloat>()) as GLsizeiptr, 
        mem::transmute(&POSITION[0]),
        gl::STATIC_DRAW);

    let loc = gl::GetAttribLocation(program, CString::new("position").unwrap().as_ptr()) as GLuint;
    gl::EnableVertexAttribArray(loc);
    gl::VertexAttribPointer(loc, 2, gl::FLOAT, gl::FALSE, 0, 0 as *const GLvoid);
    }

    position_buffer
}

fn setup_blending_program(blend_ratio: f32) {
    let program = Shader::new("passthrough.vsh", "blend.fsh");
    let id = program.ID;
    unsafe {
        program.useProgram();
        println!("id: {}. location of tex1: {} tex2: {} blendFactor: {}", id,
            gl::GetUniformLocation(id, to_string2("tex1").as_ptr()),
            gl::GetUniformLocation(id, to_string2("tex2").as_ptr()),
            gl::GetUniformLocation(id, to_string2("blendFactor").as_ptr()));
        BLEND_PROGRAM.position_buffer = setup_position_buffer(id);
        BLEND_PROGRAM.texture_buffer = setup_texture_buffer(id);
        gl::Uniform1i(gl::GetUniformLocation(id, to_string2("tex1").as_ptr()), 0);
        gl::Uniform1i(gl::GetUniformLocation(id, to_string2("tex2").as_ptr()), 1);
        gl::Uniform1f(gl::GetUniformLocation(id, to_string2("blendFactor").as_ptr()), blend_ratio);
        BLEND_PROGRAM.program = program;
    }
}

fn setup_invert_program() {
    let program = Shader::new("passthrough.vsh", "invert.fsh");
    let id = program.ID;
    unsafe {
        program.useProgram();
        INVERT_PROGRAM.position_buffer = setup_position_buffer(id);
        INVERT_PROGRAM.texture_buffer = setup_texture_buffer(id);
        gl::ActiveTexture(gl::TEXTURE0);
        program.setInt(to_string("tex1"), 0);
        INVERT_PROGRAM.program = program;
    }
}

fn tex_setup(texture_enum: u32) -> u32
{
    unsafe {
        let mut texture = 0;
        gl::GenTextures(1, &mut texture);
        gl::ActiveTexture(texture_enum);
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
    let (mut internal_window, _events) = glfw.create_window(width as u32, height as u32, "", glfw::WindowMode::Windowed)
        .expect("Failed to create GLFW window.");
    unsafe {
        WINDOW = &internal_window;
        internal_window.make_current();
        gl::load_with(|symbol| internal_window.get_proc_address(symbol) as *const _);
        gl::Viewport(0, 0, width, height); 
        TEXTURE1 = tex_setup(gl::TEXTURE0);
        TEXTURE2 = tex_setup(gl::TEXTURE1);
    }

    setup_blending_program(blend_ratio);
    setup_invert_program();
}

fn load_texture(tex: &TextureInfo) {
    unsafe {
        gl::TexImage2D(gl::TEXTURE_2D, 0,
            gl::RGB as i32, tex.width, tex.height,
            0, gl::RGB, gl::UNSIGNED_BYTE, 
            tex.buffer as *const GLvoid);
        
    }
}

#[no_mangle]
pub extern "C" fn invertFrame(tex: TextureInfo) {
    unsafe{
        INVERT_PROGRAM.program.useProgram();
        gl::ActiveTexture(gl::TEXTURE0);
        load_texture(&tex);
        gl::DrawArrays(gl::TRIANGLES, 0, 6);
        gl::ReadPixels(0, 0, tex.width, tex.height, gl::RGB, gl::UNSIGNED_BYTE, tex.buffer as *mut GLvoid);
    }
}

#[no_mangle]
pub extern "C" fn blendFrames(target: TextureInfo, tex1: TextureInfo, tex2: TextureInfo) {
    unsafe{
        BLEND_PROGRAM.program.useProgram();
        gl::ActiveTexture(gl::TEXTURE0);
        load_texture(&tex1);
        gl::ActiveTexture(gl::TEXTURE1);
        load_texture(&tex2);
        gl::DrawArrays(gl::TRIANGLES, 0, 6);
        gl::ReadPixels(0, 0, target.width, target.height, gl::RGB, gl::UNSIGNED_BYTE, target.buffer as *mut GLvoid);
    }
}

#[no_mangle]
pub extern "C" fn tearDownOpenGL() {
    unsafe {
        gl::DeleteTextures(1, &TEXTURE1);
        gl::DeleteTextures(1, &TEXTURE2);
    }
    //println!("tearDownOpenGL");
}
