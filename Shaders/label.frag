precision highp float;

in vec2 TexCoords;
out vec4 FragColor;

uniform float iTime;
uniform sampler2D texture1;

void main() {
  vec4 c = texture(texture1, TexCoords);
  if (c.r < 0.5) {
    discard;
  }
  FragColor = vec4(c.r, c.r, c.r, 1.0);
}
