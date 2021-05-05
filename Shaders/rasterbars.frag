in vec2 TexCoord;
uniform float iTime;
out vec4 FragColor;
#define PI 3.1415926535

float addRasterbar(float old, float offset) {
  float r1 = (1.0 - TexCoord.y + sin(iTime*2.0+offset)/2.5) * PI;
  float col = pow(sin(r1),128);
  return old > 0.3 ? old : old+col;
}
void main() {

  float col;
  for (int i=0; i<6; i++) {
    col = addRasterbar(col, 0.4*i);
  }
  FragColor = vec4(0.5*col,0.3*col,col, 1.0);
}
