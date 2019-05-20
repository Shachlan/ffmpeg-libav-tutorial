uniform sampler2D tex;
varying vec2 vTexCoord;
void main() {
    const vec3 kInvert = vec3(1, 1, 1);
    gl_FragColor = vec4(kInvert - texture2D(tex, vTexCoord).rgb, 1);
}\
