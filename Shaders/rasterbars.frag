precision mediump float;
precision mediump int;

in vec2 TexCoord;
uniform float iTime;
uniform float tweak;
uniform float sineOffset;
uniform int bars;
uniform vec3 baseCol;

out vec4 FragColor;
#define PI 3.1415926535

float addRasterbar(float old, float offset) {
  float r1 = (1.0 - TexCoord.y + sin(iTime*2.0+offset+sineOffset - 2.3 * sin(TexCoord.x*tweak+cos(iTime)))/2.5*0.8) * PI;
  float col = pow(sin(r1),64.0);
  return col > 0.2 ? col : old;
}

void main() {
  float col=0.0;
  for (int i=0; i<bars; i++) {
    col = addRasterbar(col*0.9, 0.4* float(i));
  }
  FragColor = vec4(col*baseCol.r, col*baseCol.g, col * baseCol.b, 1.0);
}
