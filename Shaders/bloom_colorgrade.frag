#ifdef GL_ES
precision highp float;
precision highp int;
#endif

in vec2 TexCoord;

uniform sampler2D texture0;
uniform vec2 screenSize;
uniform float bloomIntensity;
uniform float bloomThreshold;
uniform float colorGradeSaturation;
uniform float colorGradeContrast;
uniform float colorGradeWarmth;

out vec4 FragColor;

vec3 brightPart(vec3 color) {
  float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
  float weight = smoothstep(bloomThreshold, 1.0, luma);
  return color * weight;
}

vec3 sampleBloom(vec2 texel, vec3 centerColor) {
  vec3 bloom = brightPart(centerColor) * 0.18;

  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(1.5, 0.0)).rgb) * 0.11;
  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(-1.5, 0.0)).rgb) * 0.11;
  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(0.0, 1.5)).rgb) * 0.11;
  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(0.0, -1.5)).rgb) * 0.11;

  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(3.5, 0.0)).rgb) * 0.07;
  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(-3.5, 0.0)).rgb) * 0.07;
  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(0.0, 3.5)).rgb) * 0.07;
  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(0.0, -3.5)).rgb) * 0.07;

  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(2.5, 2.5)).rgb) * 0.07;
  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(-2.5, 2.5)).rgb) * 0.07;
  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(2.5, -2.5)).rgb) * 0.07;
  bloom += brightPart(texture(texture0, TexCoord + texel * vec2(-2.5, -2.5)).rgb) * 0.07;

  return bloom;
}

void main() {
  vec2 texel = screenSize.x > 0.0 && screenSize.y > 0.0
                   ? vec2(1.0) / screenSize
                   : vec2(0.0);
  vec4 source = texture(texture0, TexCoord);
  vec3 color = source.rgb;
  if (bloomIntensity > 0.001) {
    color += sampleBloom(texel, source.rgb) * bloomIntensity;
  }

  float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
  color = mix(vec3(luma), color, colorGradeSaturation);
  color = (color - 0.5) * colorGradeContrast + 0.5;
  color *= vec3(1.0 + colorGradeWarmth * 0.12,
                1.0 + colorGradeWarmth * 0.03,
                1.0 - colorGradeWarmth * 0.10);

  FragColor = vec4(clamp(color, 0.0, 1.0), source.a);
}
