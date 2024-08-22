precision mediump float;
precision mediump int;

#define M_PI 3.1415926535897932384626433832795

in vec2 TexCoord;
uniform float iTime;
uniform float speed = 0.0;
out vec4 FragColor;

void main() {
    vec2 uvMod = (TexCoord*2.0) - 1.0;
    vec2 polar = vec2(length(uvMod), atan(uvMod.y, uvMod.x) + M_PI);
    float spin = polar.y + iTime * speed;
    float sector = spin / (M_PI/180.0*20.0);
    float col = mod(sector, 2.0) < 1.0 ? 0.6 : 0.8;
    FragColor = vec4(col, col, col, 1.0);
}
