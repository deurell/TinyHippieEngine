#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform float time;
uniform vec4 col;

uniform sampler2D ourTexture;

void main(){
    vec4 col2 = abs(sin(time)) * col;
    //FragColor = col2;
    FragColor = texture(ourTexture, TexCoord) * col2;
}
