precision mediump float;
precision mediump int;

#define M_PI 3.1415926535897932384626433832795

in vec2 TexCoord;
uniform float iTime;
uniform sampler2D texture0;
out vec4 FragColor;

void main() {
  vec2 invertedTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
  vec4 col = texture(texture0, invertedTexCoord);
  FragColor = col;
}
