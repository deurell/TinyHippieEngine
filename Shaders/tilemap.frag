#ifdef GL_ES
precision highp float;
precision highp int;
#endif

in vec2 TexCoord;
uniform float iTime;
uniform sampler2D texture0;
out vec4 FragColor;

void main() {
  FragColor = texture(texture0, TexCoord);
}
