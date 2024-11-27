precision mediump float;
precision mediump int;

#define M_PI 3.1415926535897932384626433832795

in vec2 TexCoord;
uniform float iTime;
uniform vec4 baseColor;
out vec4 FragColor;

void main() {
    FragColor = baseColor;
}
