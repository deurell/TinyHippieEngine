precision mediump float;
precision mediump int;

#define M_PI 3.1415926535897932384626433832795

in vec2 TexCoord;
uniform float iTime;
uniform sampler2D texture0;
uniform vec2 atlasSize;
uniform vec4 atlasSourceRectPixels;
uniform vec3 atlasFlip;
out vec4 FragColor;

void main() {
  vec2 localUv = TexCoord;
  if (atlasFlip.z > 0.5) {
    localUv = vec2(localUv.y, localUv.x);
  }
  if (atlasFlip.x > 0.5) {
    localUv.x = 1.0 - localUv.x;
  }
  if (atlasFlip.y > 0.5) {
    localUv.y = 1.0 - localUv.y;
  }

  vec2 invertedTexCoord = vec2(localUv.x, 1.0 - localUv.y);
  vec2 sampleUv = invertedTexCoord;
  if (atlasSourceRectPixels.z > 0.0 && atlasSourceRectPixels.w > 0.0 &&
      atlasSize.x > 0.0 && atlasSize.y > 0.0) {
    vec2 pixel = atlasSourceRectPixels.xy +
                 vec2(localUv.x, 1.0 - localUv.y) *
                     atlasSourceRectPixels.zw;
    sampleUv = pixel / atlasSize;
  }

  vec4 col = texture(texture0, sampleUv);
  FragColor = col;
}
