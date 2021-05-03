precision highp float;

in vec2 TexCoords;
in vec4 WorldPos;
out vec4 FragColor;

uniform float iTime;
uniform sampler2D texture1;

void main() {
  vec4 c = texture(texture1, TexCoords);
  if (c.r < 0.8) {
    discard;
  }
  float col = 1.0;
  FragColor = vec4(col, col, col, 1.0);
}
