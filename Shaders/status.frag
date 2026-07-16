precision highp float;

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D texture1;
uniform vec4 textColor;

void main() {
  float glyphAlpha = texture(texture1, TexCoords).r;
  if (glyphAlpha < 0.01) {
    discard;
  }

  FragColor = vec4(textColor.rgb, textColor.a * glyphAlpha);
}
