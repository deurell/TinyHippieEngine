layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec4 WorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float iTime;
uniform float amp;
uniform float freq;

void main() {
  TexCoords = aTexCoords;
  WorldPos = model * vec4(aPos, 1);
  vec3 pos = aPos;
  pos.x += amp * sin(iTime * freq);
  gl_Position = projection * view * model * vec4(pos, 1);
}
