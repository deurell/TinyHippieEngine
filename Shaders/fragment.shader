#version 330 core
out vec4 FragColor;
uniform float time;
uniform vec4 col;

void main(){
    vec4 col2 = abs(sin(time)) * col;

    FragColor = vec4(col2.x, col2.y, col2.z, 1.0f);
}
