layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in uvec4 aJoints;
layout(location = 4) in vec4 aWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int useSkinning;
uniform mat4 boneMatrices[64];

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
  vec4 skinnedPos = vec4(aPos, 1.0);
  vec3 skinnedNormal = aNormal;
  if (useSkinning != 0) {
    mat4 skinMatrix =
        aWeights.x * boneMatrices[aJoints.x] +
        aWeights.y * boneMatrices[aJoints.y] +
        aWeights.z * boneMatrices[aJoints.z] +
        aWeights.w * boneMatrices[aJoints.w];
    skinnedPos = skinMatrix * vec4(aPos, 1.0);
    skinnedNormal = mat3(skinMatrix) * aNormal;
  }

  vec4 worldPos = model * skinnedPos;
  FragPos = worldPos.xyz;
  Normal = mat3(transpose(inverse(model))) * skinnedNormal;
  TexCoord = aTexCoord;
  gl_Position = projection * view * worldPos;
}
