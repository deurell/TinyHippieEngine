precision mediump float;
precision mediump int;

in vec2 TexCoord;
uniform float iTime;
out vec4 FragColor;

void main() {
    float col = abs(sin(iTime));
    FragColor = vec4(col, col, col, 1.0);
}
