layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec2 FragPos;
out vec4 WorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float iTime;

void main() {
  TexCoords = aTexCoords;
  WorldPos = model * vec4(aPos, 1);
  float w_x = WorldPos.x;
  vec3 pos = aPos;
  pos.x -= iTime * 150.0;

  pos.y = pos.y + 48.0 * sin(-iTime * 0.76 + pos.x * 0.013);
  pos.x = pos.x + 24.0 * cos(iTime * 0.69 + pos.y * 0.012);
  pos.z = pos.z + 96.0 * cos(-iTime * 0.37 + pos.x * 0.011);

  gl_Position = projection * view * model * vec4(pos, 1);

  vec2 ndc = gl_Position.xy / gl_Position.w;
  FragPos = ndc.xy * 0.5 + 0.5;
}
