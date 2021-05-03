layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec4 WorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float iTime;

void main() {
  TexCoords = aTexCoords;
  WorldPos = model * vec4(aPos, 1);
  vec3 pos = aPos;
  pos.x += 72.0 * sin(iTime * 1.2);

  gl_Position = projection * view * model * vec4(pos, 1);
}
