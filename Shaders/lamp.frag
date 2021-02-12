precision highp float;
precision highp int;

uniform float iTime;
uniform vec3 objectColor;
uniform vec3 lightColor;

out vec4 FragColor;

void main() {
  float col = 0.75 + 0.25 * sin(iTime * 4.0);
  FragColor = vec4(col, col, col, 1.0f);
}
