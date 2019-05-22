 #version 410

uniform sampler2D tex1;
uniform sampler2D tex2;
uniform float blendFactor;
in vec2 vTexCoord;
out vec4 FragColor;

void main() {
    vec4 color1 = texture(tex1, vTexCoord);
    vec4 color2 = texture(tex2, vTexCoord);
    FragColor = (color1 * blendFactor) + (color2 * (1.0 - blendFactor));
}
