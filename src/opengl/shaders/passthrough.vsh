 #version 410

in vec2 position;
in vec2 texCoord;
out vec2 vTexCoord;
void main(void) {
    gl_Position = vec4(position, 0, 1);
    vTexCoord = texCoord;
}