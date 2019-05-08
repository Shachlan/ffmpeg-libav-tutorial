uniform sampler2D tex1;
uniform sampler2D tex2;
varying vec2 vTexCoord;

uniform int blendMode;
uniform float opacity;

const int kBlendModeNormal = 0;
const int kBlendModeDarken = 1;
const int kBlendModeMultiply = 2;
const int kBlendModeHardLight = 3;
const int kBlendModeSoftLight = 4;
const int kBlendModeLighten = 5;
const int kBlendModeScreen = 6;
const int kBlendModeColorBurn = 7;
const int kBlendModeOverlay = 8;

void normal(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  gl_FragColor.rgb = Sca + Dca * (1.0 - Sa);
  gl_FragColor.a = Sa + Da - Sa * Da;
}

void darken(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  gl_FragColor.rgb = min(Sca * Da, Dca * Sa) + Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  gl_FragColor.a = Sa + Da - Sa * Da;
}

void multiply(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  gl_FragColor.rgb = Sca * Dca + Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  gl_FragColor.a = Sa + Da - Sa * Da;
}

void hardLight(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  vec3 below = 2.0 * Sca * Dca + Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  vec3 above = Sca * (1.0 + Da) + Dca * (1.0 + Sa) - Sa * Da - 2.0 * Sca * Dca;

  gl_FragColor.rgb = mix(below, above, step(0.5 * Sa, Sca));
  gl_FragColor.a = Sa + Da - Sa * Da;
}

void softLight(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  float safeDa = Da + step(Da, 0.0);

  vec3 below = 2.0 * Sca * Dca + Dca * (Dca / safeDa) * (Sa - 2.0 * Sca) + Sca * (1.0 - Da)
      + Dca * (1.0 - Sa);
  vec3 above = 2.0 * Dca * (Sa - Sca) + sqrt(Dca * Da) * (2.0 * Sca - Sa) +
      Sca * (1.0 - Da) + Dca * (1.0 - Sa);

  gl_FragColor.rgb = mix(below, above, step(0.5 * Sa, Sca));
  gl_FragColor.a = Sa + Da - Sa * Da;
}

void lighten(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  gl_FragColor.rgb = max(Sca * Da, Dca * Sa) + Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  gl_FragColor.a = Sa + Da - Sa * Da;
}

void screen(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  gl_FragColor.rgb = Sca + Dca - Sca * Dca;
  gl_FragColor.a = Sa + Da - Sa * Da;
}

void colorBurn(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  float safeDa = Da + step(Da, 0.0);
  vec3 stepSca = step(Sca, vec3(0.0));
  vec3 safeSca = Sca + stepSca;

  vec3 zero = Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  vec3 nonzero = Sa * Da * (vec3(1.0) - min(vec3(1.0), (1.0 - Dca / safeDa) * Sa / safeSca))
      + Sca * (1.0 - Da) + Dca * (1.0 - Sa);

  gl_FragColor.rgb = mix(zero, nonzero, 1.0 - stepSca);
  gl_FragColor.a = Sa + Da - Sa * Da;
}

void overlay(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  vec3 below = 2.0 * Sca * Dca + Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  vec3 above = Sca * (1.0 + Da) + Dca * (1.0 + Sa) - 2.0 * Dca * Sca - Da * Sa;

  gl_FragColor.rgb = mix(below, above, step(0.5 * Da, Dca));
  gl_FragColor.a = Sa + Da - Sa * Da;
}

void main() {
    vec4 back = texture2D(tex1, vTexCoord);
    vec4 front = texture2D(tex2, vTexCoord);

  // Define variables as they appear in SVG spec. See http://www.w3.org/TR/SVGCompositing/.
  vec3 Sca = front.rgb * front.a;
  vec3 Dca = back.rgb * back.a;
  float Sa = front.a;
  float Da = back.a;

  if (blendMode == kBlendModeNormal) {
    normal(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeDarken) {
    darken(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeMultiply) {
    multiply(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeHardLight) {
    hardLight(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeSoftLight) {
    softLight(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeLighten) {
    lighten(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeScreen) {
    screen(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeColorBurn) {
    colorBurn(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeOverlay) {
    overlay(Sca, Dca, Sa, Da);
  } else {
    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
  }

  gl_FragColor = mix(vec4(Dca, Da), gl_FragColor, opacity);
}
