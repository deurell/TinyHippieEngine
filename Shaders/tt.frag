precision lowp float;
precision lowp int;

in vec3 ourColor;
in vec2 TexCoord;
out vec4 FragColor;

uniform float iTime;
uniform sampler2D texture1;

void main() {
  vec2 uv = vec2(TexCoord.x, 1.0 - TexCoord.y);
  vec4 tCol = texture(texture1, uv).rgba;
  FragColor = vec4(tCol.r, tCol.r, tCol.r, tCol.a);
}