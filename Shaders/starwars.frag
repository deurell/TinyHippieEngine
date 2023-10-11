precision highp float;

in vec2 TexCoords;
in vec2 FragPos;
in vec4 WorldPos;
out vec4 FragColor;

uniform float iTime;
uniform sampler2D texture1;
uniform mat4 view;
uniform float c1;
uniform float c2;

#define PI 3.14159265

void main() {
  vec4 c = texture(texture1, TexCoords);
  if (c.r < 0.8) {
    discard;
  }

  // Calculate the distance from the camera
  vec4 cameraPos = inverse(view) * vec4(0, 0, 6, 1);
  float distance = length(WorldPos.xyz - cameraPos.xyz);

  // Darken the color based on the distance
  float darkness = distance * c1 - c2;
  //float darkness = distance * 0.03 - 1.35;
  FragColor = vec4(1.0 - darkness, 0.9098 - darkness, 0.1216 - darkness, 1.0);
}
