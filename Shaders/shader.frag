precision highp float;
precision highp int;

uniform float iTime;
uniform vec3 objectColor;
uniform vec3 lightColor;

out vec4 FragColor;

void main() {
  FragColor = vec4((lightColor * objectColor), 1.0);
}
