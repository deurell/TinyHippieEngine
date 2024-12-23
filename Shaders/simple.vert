layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

uniform float iTime;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;

void main() {
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1);
}
