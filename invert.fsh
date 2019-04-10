uniform sampler2D tex;
varying vec2 vTexCoord;
void main() {
    const vec4 kInvert = vec4(1, 1, 1, 0);
    gl_FragColor = kInvert - texture2D(tex, vTexCoord);
}