precision mediump float;
precision mediump int;

in vec2 TexCoord;
uniform float iTime;
uniform float sineOffset;
out vec4 FragColor;

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void main() {
  float col = 0.0;
  float val = lerp(0.0,3.1415926535 * 4.0, TexCoord.y);
  col = sin(val);
  col = pow(col,2.0);
  FragColor = vec4(col*0.1, col*0.1, col, 1.0);
}
