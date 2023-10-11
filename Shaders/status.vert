layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec2 FragPos;
out vec4 WorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float iTime;
uniform float rotAngle1;
uniform float rotAngle2;
uniform float c1;
uniform float c2;

void main() {
    TexCoords = aTexCoords;

    // Calculate the World Position before rotation
    WorldPos = model * vec4(aPos, 1.0);

    // Calculate the angle based on z value of WorldPos
    float angle = sin(iTime*0.5 + WorldPos.y * rotAngle1) * rotAngle2;

    // Rotation matrix around y-axis
    mat4 rotationY = mat4(
        cos(angle), 0, sin(angle), 0,
        0, 1, 0, 0,
        -sin(angle), 0, cos(angle), 0,
        0, 0, 0, 1
    );

    // Apply the rotation
    vec3 rotatedPos = vec3(rotationY * WorldPos);

    gl_Position = projection * view * vec4(rotatedPos, 1.0);

    vec2 ndc = gl_Position.xy / gl_Position.w;
    FragPos = ndc.xy * 0.5 + 0.5;
}
