precision mediump float;
precision mediump int;

#define M_PI 3.1415926535897932384626433832795

in vec2 TexCoord;
uniform float iTime;
out vec4 FragColor;

void main() {
    vec2 uvMod = (TexCoord*2.0) - 1.0;
    float polar = (atan(uvMod.y, uvMod.x) + M_PI) + iTime;
    float sector = polar / (M_PI/180.0*20.0);
    float col = mod(sector, 2.0) < 1.0 ? 0.0 : 1.0;
    FragColor = vec4(col, col, col, 1.0);
}
