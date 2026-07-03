in vec2 TexCoord;

uniform sampler2D texture0;
uniform vec2 screenSize;
uniform float chromaticAberrationStrength;

out vec4 FragColor;

void main() {
  vec2 texelSize = screenSize.x > 0.0 && screenSize.y > 0.0
                       ? vec2(1.0) / screenSize
                       : vec2(0.0);
  vec2 centeredUv = TexCoord - vec2(0.5);
  vec2 offset = centeredUv * chromaticAberrationStrength;
  float red = texture(texture0, TexCoord + offset + texelSize).r;
  float green = texture(texture0, TexCoord).g;
  float blue = texture(texture0, TexCoord - offset - texelSize).b;
  float alpha = texture(texture0, TexCoord).a;
  FragColor = vec4(red, green, blue, alpha);
}
