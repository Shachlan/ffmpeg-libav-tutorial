attribute vec2 position;
attribute vec2 texCoord1;
varying vec2 vTexCoord1;

attribute vec2 texCoord2;
varying vec2 vTexCoord2;

void main(void) {
    gl_Position = vec4(position, 0, 1);
    vTexCoord1 = texCoord1;
    vTexCoord2 = texCoord2;
}
