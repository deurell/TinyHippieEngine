#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform float time;
uniform vec4 col;

uniform sampler2D ourTexture;

void main(){
    float pi = 3.1415926535;

    vec4 col2 = abs(sin(time)) * col;
    //FragColor = col2;
    float u = TexCoord.x + sin(0.7*time);
    float v = TexCoord.y + sin(0.5*time+pi/2);

    FragColor = texture(ourTexture, vec2(u,v)) * col2;
}
