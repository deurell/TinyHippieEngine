#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform float time;
uniform float rasterwidth;
uniform vec4 col;
uniform float speed;

uniform sampler2D ourTexture;

vec3 random3(vec3 c) {
    float j = 4096.0*sin(dot(c, vec3(17.0, 59.4, 15.0)));
    vec3 r;
    r.z = fract(512.0*j);
    j *= .125;
    r.x = fract(512.0*j);
    j *= .125;
    r.y = fract(512.0*j);
    return r-0.5;
}

const float F3 =  0.3333333;
const float G3 =  0.1666667;

float simplex3d(vec3 p) {
    vec3 s = floor(p + dot(p, vec3(F3)));
    vec3 x = p - s + dot(s, vec3(G3));

    vec3 e = step(vec3(0.0), x - x.yzx);
    vec3 i1 = e*(1.0 - e.zxy);
    vec3 i2 = 1.0 - e.zxy*(1.0 - e);

    vec3 x1 = x - i1 + G3;
    vec3 x2 = x - i2 + 2.0*G3;
    vec3 x3 = x - 1.0 + 3.0*G3;

    vec4 w, d;

    w.x = dot(x, x);
    w.y = dot(x1, x1);
    w.z = dot(x2, x2);
    w.w = dot(x3, x3);

    w = max(0.6 - w, 0.0);

    d.x = dot(random3(s), x);
    d.y = dot(random3(s + i1), x1);
    d.z = dot(random3(s + i2), x2);
    d.w = dot(random3(s + 1.0), x3);

    w *= w;
    w *= w;
    d *= w;
    return dot(d, vec4(52.0));
}

float noise(vec3 v) {
    return 0.3333333 * simplex3d(v)
    + 0.4666667 * simplex3d(1.2 * v)
    + 0.127 * simplex3d(1.3 * v)+
    + 0.1666667 * simplex3d(1.5 * v);
}

void main() {
    vec2 txc = TexCoord;
    vec2 uv = txc * 2. - 1.;
    vec3 txc_time = vec3(txc, time * speed);

    float intensity = noise(vec3(txc_time * 1.3) + 23.5);
    float y = abs(intensity + uv.y);
    float g = pow(y, rasterwidth);

    vec3 color = vec3(1.+col.x, 1.+col.y, 1.+col.z);
    color = color * -g + color;
    color = color * color;
    color = color * color;

    FragColor.rgb = color;
    FragColor.w = 1.;
}
