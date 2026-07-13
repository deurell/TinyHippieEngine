#ifdef GL_ES
precision highp float;
precision highp int;
#endif

in vec2 TexCoord;

uniform vec4 lightColor;
uniform float intensity;
uniform float softness;

out vec4 FragColor;

void main() {
  vec2 centered = TexCoord * 2.0 - vec2(1.0);
  float distanceFromCenter = length(centered);
  float falloffPower = mix(4.0, 1.15, clamp(softness, 0.0, 1.0));
  float glow = pow(clamp(1.0 - distanceFromCenter, 0.0, 1.0), falloffPower);
  float alpha = glow * lightColor.a * intensity;
  FragColor = vec4(lightColor.rgb * alpha, alpha);
}
