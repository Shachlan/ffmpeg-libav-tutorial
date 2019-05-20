uniform sampler2D tex1;
uniform sampler2D tex2;
uniform float blendFactor;
varying vec2 vTexCoord;

void main() {
    vec4 color1 = texture2D(tex1, vTexCoord);
    vec4 color2 = texture2D(tex2, vTexCoord);
    gl_FragColor = (color1 * blendFactor) + (color2 * (1.0 - blendFactor));
}
