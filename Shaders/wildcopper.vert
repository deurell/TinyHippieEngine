layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec4 WorldPos;

uniform mat4 mvp;
uniform float iTime;
uniform float deg;

void main() {
  TexCoords = aTexCoords;
  gl_Position = mvp * vec4(aPos, 1);
}
