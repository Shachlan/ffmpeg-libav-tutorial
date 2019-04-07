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

static mut invert_program: ProgramInfo = ProgramInfo {
        program: Shader { ID: 0 }, position_buffer:0, texture_buffer:0
    };
static mut blend_program: ProgramInfo = ProgramInfo {
        program: Shader { ID: 0 }, position_buffer:0, texture_buffer:0
    };

static mut window: *const glfw::Window = std::ptr::null();
static mut texture1: u32 = 0;
static mut texture2: u32 = 0;

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
    let ID = program.ID;
    unsafe {
        blend_program.position_buffer = setup_position_buffer(ID);
        blend_program.texture_buffer = setup_texture_buffer(ID);
        program.setInt(to_string("tex1"), texture1 as i32);
        program.setInt(to_string("tex2"), texture2 as i32);
        program.setFloat(to_string("blendFactor"), blend_ratio);
        blend_program.program = program;
    }
}

fn setup_invert_program() {
    let program = Shader::new("passthrough.vsh", "invert.fsh");
    let ID = program.ID;
    unsafe {
        invert_program.position_buffer = setup_position_buffer(ID);
        invert_program.texture_buffer = setup_texture_buffer(ID);
        program.setInt(to_string("tex1"), texture1 as i32);
        invert_program.program = program;
    }
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
    let (mut internal_window, events) = glfw.create_window(width as u32, height as u32, "", glfw::WindowMode::Windowed)
        .expect("Failed to create GLFW window.");
    unsafe {
        window = &internal_window;
        internal_window.make_current();
        gl::load_with(|symbol| internal_window.get_proc_address(symbol) as *const _);
        gl::Viewport(0, 0, width, height); 
        texture1 = tex_setup();
        texture2 = tex_setup();
    }

    setup_blending_program(blend_ratio);
    setup_invert_program();
}

#[no_mangle]
pub extern "C" fn invertFrame(tex: TextureInfo) {
    unsafe{
        invert_program.program.useProgram();
    }
}

#[no_mangle]
pub extern "C" fn blendFrames(target: TextureInfo, tex1: TextureInfo, tex2: TextureInfo) {
    unsafe{
        blend_program.program.useProgram();
    }
}

#[no_mangle]
pub extern "C" fn tearDownOpenGL() {
    unsafe {
        gl::DeleteTextures(1, &texture1);
        gl::DeleteTextures(1, &texture2);
    }
    //println!("tearDownOpenGL");
}
