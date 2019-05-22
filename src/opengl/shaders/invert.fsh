 #version 410

uniform sampler2D tex;
in vec2 vTexCoord;
out vec4 FragColor;

void main() {
    const vec3 kInvert = vec3(1, 1, 1);
    FragColor = vec4(kInvert - texture(tex, vTexCoord).rgb, 1);
}\
