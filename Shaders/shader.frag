precision highp float;
precision highp int;

in vec3 ourColor;
in vec2 TexCoord;

uniform float iTime;
uniform sampler2D texture1;

out vec4 FragColor;

void main() {
  vec3 texCol = texture(texture1, TexCoord).rgb;
  FragColor = vec4(texCol, 1);
}
