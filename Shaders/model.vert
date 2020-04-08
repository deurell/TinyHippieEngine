
struct Material {
  sampler2D diffuse;
  vec3 diffuseFallback;
  vec3 ambientFallback;
  vec3 specular;
  float shininess;
  int id;
};

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform Material material;
uniform float iTime;

void main() {
  TexCoords = aTexCoords;
  Normal = mat3(transpose(inverse(model))) * aNormal;
  FragPos = vec3(model * vec4(aPos, 1.0));
  if (material.id == 4) {
    vec4 newPos = vec4(aPos.x,
                       (aPos.y + 0.1 + 0.2 * sin(iTime + aPos.z) +
                        0.2 * cos(iTime * -0.8 - aPos.x)),
                       aPos.z, 1.0);
    gl_Position = projection * view * model * newPos;
  } else {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
  }
}
