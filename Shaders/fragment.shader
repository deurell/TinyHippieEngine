#version 330 core
out vec4 FragColor;
uniform float time;
void main(){
    float col = abs(sin(time));
    FragColor = vec4(col, col, col, 1.0f);
}
