precision highp float;
in vec2 TexCoords;
in vec4 WorldPos;
out vec4 FragColor;

uniform float iTime;
uniform sampler2D texture1;
uniform float deg;

float lerp(float a,float b,float t) {
    return a + (b-a) * t;
}

void main() {
  vec4 c = texture(texture1, TexCoords);
  if (c.r < 0.8) {
    discard;
  }
  float col = 1.0;
  float normalizedAngle = deg / (2.0 * 3.1415926535);
  float val = lerp(0.0,3.1415926535,normalizedAngle);
  col = sin(val);
  FragColor = vec4(col, col, 0.0, 1.0);
}
