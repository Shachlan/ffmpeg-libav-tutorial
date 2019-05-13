uniform sampler2D tex1;
varying vec2 vTexCoord1;
uniform float opacity1;

uniform sampler2D tex2;
varying vec2 vTexCoord2;
uniform float opacity2;
uniform int blendMode2;

// uniform sampler2D tex3;
// varying vec2 vTexCoord3;
// uniform float opacity3;
// uniform int blendMode3;

// uniform sampler2D tex4;
// varying vec2 vTexCoord4;
// uniform float opacity4;
// uniform int blendMode4;

// uniform sampler2D tex5;
// varying vec2 vTexCoord5;
// uniform float opacity5;
// uniform int blendMode5;

// uniform sampler2D tex6;
// varying vec2 vTexCoord6;
// uniform float opacity6;
// uniform int blendMode6;

// uniform sampler2D tex7;
// varying vec2 vTexCoord7;
// uniform float opacity7;
// uniform int blendMode7;

// uniform sampler2D tex8;
// varying vec2 vTexCoord8;
// uniform float opacity8;
// uniform int blendMode8;

uniform int numberOfLayers;

const int kBlendModeNormal = 0;
const int kBlendModeDarken = 1;
const int kBlendModeMultiply = 2;
const int kBlendModeHardLight = 3;
const int kBlendModeSoftLight = 4;
const int kBlendModeLighten = 5;
const int kBlendModeScreen = 6;
const int kBlendModeColorBurn = 7;
const int kBlendModeOverlay = 8;

vec4 normal(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  vec3 rgb = Sca + Dca * (1.0 - Sa);
  float alpha = Sa + Da - Sa * Da;
  return vec4(rgb, alpha);
}

vec4 darken(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  vec3 rgb = min(Sca * Da, Dca * Sa) + Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  float alpha = Sa + Da - Sa * Da;
  return vec4(rgb, alpha);
}

vec4 multiply(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  vec3 rgb = Sca * Dca + Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  float alpha = Sa + Da - Sa * Da;
  return vec4(rgb, alpha);
}

vec4 hardLight(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  vec3 below = 2.0 * Sca * Dca + Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  vec3 above = Sca * (1.0 + Da) + Dca * (1.0 + Sa) - Sa * Da - 2.0 * Sca * Dca;

  vec3 rgb = mix(below, above, step(0.5 * Sa, Sca));
  float alpha = Sa + Da - Sa * Da;
  return vec4(rgb, alpha);
}

vec4 softLight(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  float safeDa = Da + step(Da, 0.0);

  vec3 below = 2.0 * Sca * Dca + Dca * (Dca / safeDa) * (Sa - 2.0 * Sca) + Sca * (1.0 - Da)
      + Dca * (1.0 - Sa);
  vec3 above = 2.0 * Dca * (Sa - Sca) + sqrt(Dca * Da) * (2.0 * Sca - Sa) +
      Sca * (1.0 - Da) + Dca * (1.0 - Sa);

  vec3 rgb = mix(below, above, step(0.5 * Sa, Sca));
  float alpha = Sa + Da - Sa * Da;
  return vec4(rgb, alpha);
}

vec4 lighten(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  vec3 rgb = max(Sca * Da, Dca * Sa) + Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  float alpha = Sa + Da - Sa * Da;
  return vec4(rgb, alpha);
}

vec4 screen(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  vec3 rgb = Sca + Dca - Sca * Dca;
  float alpha = Sa + Da - Sa * Da;
  return vec4(rgb, alpha);
}

vec4 colorBurn(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  float safeDa = Da + step(Da, 0.0);
  vec3 stepSca = step(Sca, vec3(0.0));
  vec3 safeSca = Sca + stepSca;

  vec3 zero = Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  vec3 nonzero = Sa * Da * (vec3(1.0) - min(vec3(1.0), (1.0 - Dca / safeDa) * Sa / safeSca))
      + Sca * (1.0 - Da) + Dca * (1.0 - Sa);

  vec3 rgb = mix(zero, nonzero, 1.0 - stepSca);
  float alpha = Sa + Da - Sa * Da;
  return vec4(rgb, alpha);
}

vec4 overlay(in vec3 Sca, in vec3 Dca, in float Sa, in float Da) {
  vec3 below = 2.0 * Sca * Dca + Sca * (1.0 - Da) + Dca * (1.0 - Sa);
  vec3 above = Sca * (1.0 + Da) + Dca * (1.0 + Sa) - 2.0 * Dca * Sca - Da * Sa;

  vec3 rgb = mix(below, above, step(0.5 * Da, Dca));
  float alpha = Sa + Da - Sa * Da;
  return vec4(rgb, alpha);
}

vec4 blend(vec4 color, int blendMode, vec4 front, float opacity) {
  vec3 Sca = front.rgb * front.a;
  vec3 Dca = color.rgb * color.a;
  float Sa = front.a;
  float Da = color.a;

  if (blendMode == kBlendModeNormal) {
    color = normal(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeDarken) {
    color = darken(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeMultiply) {
    color = multiply(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeHardLight) {
    color = hardLight(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeSoftLight) {
    color = softLight(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeLighten) {
    color = lighten(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeScreen) {
    color = screen(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeColorBurn) {
    color = colorBurn(Sca, Dca, Sa, Da);
  } else if (blendMode == kBlendModeOverlay) {
    color = overlay(Sca, Dca, Sa, Da);
  } else {
    color = vec4(0.0, 0.0, 0.0, 1.0);
  }
  return mix(vec4(Dca, Da), color, opacity);
}

void main() {
  vec4 color = texture2D(tex1, vTexCoord) * opacity1;

  if (numberOfLayers >= 2) {
    color = blend(color, blendMode2, texture2D(tex2, vTexCoord), opacity2);
  }

  // if (numberOfLayers >= 3) {
  //   color = blend(color, blendMode3, texture2D(tex3, vTexCoord3), opacity3);
  // }


  // if (numberOfLayers >= 4) {
  //   color = blend(color, blendMode4, texture2D(tex4, vTexCoord4), opacity4);
  // }


  // if (numberOfLayers >= 5) {
  //   color = blend(color, blendMode5, texture2D(tex5, vTexCoord5), opacity5);
  // }


  // if (numberOfLayers >= 6) {
  //   color = blend(color, blendMode6, texture2D(tex6, vTexCoord6), opacity6);
  // }


  // if (numberOfLayers >= 7) {
  //   color = blend(color, blendMode7, texture2D(tex7, vTexCoord7), opacity7);
  // }


  // if (numberOfLayers >= 8) {
  //   color = blend(color, blendMode8, texture2D(tex8, vTexCoord8), opacity8);
  // }

  gl_FragColor = color;
}
