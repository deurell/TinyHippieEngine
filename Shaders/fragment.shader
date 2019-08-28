#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform float iTime;
uniform sampler2D texture1;
uniform sampler2D texture2;

in float color;
out vec4 frag_color;

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

//const int indexMatrix4x4[16] = int[](0,  8,  2,  10,
//12, 4,  14, 6,
//3,  11, 1,  9,
//15, 7,  13, 5);
//
//float indexValue() {
//    int x = int(mod(gl_FragCoord.x, 4));
//    int y = int(mod(gl_FragCoord.y, 4));
//    return indexMatrix4x4[(x + y * 4)] / 16.0;
//}

const int indexMatrix8x8[64] = int[](
    0,  32, 8,  40, 2,  34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44, 4,  36, 14, 46, 6,  38,
    60, 28, 52, 20, 62, 30, 54, 22,
    3,  35, 11, 43, 1,  33, 9,  41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47, 7,  39, 13, 45, 5,  37,
    63, 31, 55, 23, 61, 29, 53, 21);

float indexValue() {
    int x = int(mod(gl_FragCoord.x, 8));
    int y = int(mod(gl_FragCoord.y, 8));
    return indexMatrix8x8[(x + y * 8)] / 64.0;
}

float colDistance(vec3 col1, vec3 col2) {
    col1.x *= 3.14159265356 * 2.0;
    col2.x *= 3.14159265356 * 2.0;

    float x = pow((sin(col1.x)*col1.y*col1.z - sin(col2.x)*col2.y*col2.z),2);
    float y = pow((cos(col1.x)*col1.y*col1.z - cos(col2.x) *col2.y*col2.z),2);
    float z = pow((col1.z - col2.z),2);
    float dist = sqrt(x+y+z);


    //float dh = min(abs(col1.x-col2.x), 1.0-abs(col1.x-col2.x));
    //float ds = abs(col1.y-col2.y);
    //float dv = abs(col1.z-col2.z);
    //float dist = length(vec3(dh,ds,dv));
    return dist;
}

vec3[2] closestColors(vec3 col) {

    vec3 palette[16] = vec3[](
    vec3( 0.0/255.0, 0.0/255.0, 0.0/255.0 ),
    vec3( 255.0/255.0, 255.0/255.0, 255.0/255.0 ),
    vec3( 104.0/255.0, 55.0/255.0, 43.0/255.0 ),
    vec3( 112.0/255.0, 164.0/255.0, 178.0/255.0 ),
    vec3( 111.0/255.0, 61.0/255.0, 133.6/255.0 ),
    vec3( 88.0/255.0, 141.0/255.0, 67.0/255.0 ),
    vec3( 53.0/255.0, 40.0/255.0, 121.0/255.0 ),
    vec3( 184.0/255.0, 199.0/255.0, 111.0/255.0 ),
    vec3( 111.0/255.0, 79.0/255.0, 37.0/255.0 ),
    vec3( 67.0/255.0, 57.0/255.0, 0.0/255.0 ),
    vec3( 154.0/255.0, 103.0/255.0, 89.0/255.0 ),
    vec3( 68.0/255.0, 68.0/255.0, 68.0/255.0 ),
    vec3( 108.0/255.0, 108.0/255.0, 108.0/255.0 ),
    vec3( 154.0/255.0, 210.0/255.0, 132.0/255.0 ),
    vec3( 108.0/255.0, 94.0/255.0, 181.0/255.0 ),
    vec3( 149.0/255.0, 149.0/255.0, 149.0/255.0 )
    );

    vec3 ret[2];
    vec3 closest = vec3(-2, 0, 0);
    vec3 secondClosest = vec3(-2, 0, 0);
    vec3 temp;
    for (int i = 0; i < 16; ++i) {
        temp = rgb2hsv(palette[i]);
        float tempDistance = colDistance(temp, col);
        if (tempDistance < colDistance(closest, col)) {
            secondClosest = closest;
            closest = temp;
        } else {
            if (tempDistance < colDistance(secondClosest, col)) {
                secondClosest = temp;
            }
        }
    }
    ret[0] = closest;
    ret[1] = secondClosest;
    return ret;
}

vec3 dither(vec3 color) {
    vec3 hslCol = rgb2hsv(color);

    vec3 cs[2] = closestColors(hslCol);
    vec3 c1 = cs[0];
    vec3 c2 = cs[1];
    float d = indexValue();
    float diff = colDistance(hslCol, c1) / (colDistance(hslCol, c1) + colDistance(hslCol,c2));

    vec3 resultColor = (diff < d) ? c1 : c2;
    return hsv2rgb(resultColor);
}

void main () {
    //const vec3 weight = vec3(0.2125, 0.7154, 0.0721);
    vec3 sourceCol = texture(texture1, TexCoord).rgb;
    //float lum = dot(sourceCol, weight);
    //vec4 greyScale = vec4(lum, lum, lum,1);
    //FragColor = greyScale;

    FragColor = vec4(dither(sourceCol),1);
}