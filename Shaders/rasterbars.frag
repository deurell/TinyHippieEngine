in vec2 TexCoord;
uniform float iTime;
out vec4 FragColor;

void main() {
  float  y = smoothstep(0.8, 0.2, TexCoord.y);
  vec3 col = vec3(y);
  FragColor = vec4(col, 1.0);
}
