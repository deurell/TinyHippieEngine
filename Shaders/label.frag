precision highp float;

in vec2 TexCoords;
in vec2 FragPos;
in vec4 WorldPos;
out vec4 FragColor;

uniform float iTime;
uniform sampler2D texture1;

void main() {
  vec4 c = texture(texture1, TexCoords);
  if (c.r < 0.5) {
    discard;
  }
  float col = mix(sin(FragPos.x * 3.1415926), sin(FragPos.y * 3.1415926), 0.5);
  FragColor = vec4(col, col, col, 1.0);
}
