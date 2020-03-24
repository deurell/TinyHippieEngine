out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform float iTime;
uniform sampler2D texture1;

in float color;
out vec4 frag_color;

vec3 random3(vec3 c) {
  float j = 4096.0 * sin(dot(c, vec3(17.0, 59.4, 15.0)));
  vec3 r;
  r.z = fract(512.0 * j);
  j *= .125;
  r.x = fract(512.0 * j);
  j *= .125;
  r.y = fract(512.0 * j);
  return r - 0.5;
}

const float F3 = 0.3333333;
const float G3 = 0.1666667;

float simplex3d(vec3 p) {
  vec3 s = floor(p + dot(p, vec3(F3)));
  vec3 x = p - s + dot(s, vec3(G3));

  vec3 e = step(vec3(0.0), x - x.yzx);
  vec3 i1 = e * (1.0 - e.zxy);
  vec3 i2 = 1.0 - e.zxy * (1.0 - e);

  vec3 x1 = x - i1 + G3;
  vec3 x2 = x - i2 + 2.0 * G3;
  vec3 x3 = x - 1.0 + 3.0 * G3;

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
  return 0.3333333 * simplex3d(v) + 0.4666667 * simplex3d(1.2 * v) +
         0.127 * simplex3d(1.3 * v) + +0.1666667 * simplex3d(1.5 * v);
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 rgb2hsv(vec3 c) {
  vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
  vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

  float d = q.x - min(q.w, q.y);
  float e = 1.0e-10;
  return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// const int indexMatrix4x4[16] = int[](0,  8,  2,  10,
// 12, 4,  14, 6,
// 3,  11, 1,  9,
// 15, 7,  13, 5);
//
// float indexValue() {
//    int x = int(mod(gl_FragCoord.x, 4));
//    int y = int(mod(gl_FragCoord.y, 4));
//    return indexMatrix4x4[(x + y * 4)] / 16.0;
//}

const int indexMatrix8x8[64] =
    int[](0, 32, 8, 40, 2, 34, 10, 42, 48, 16, 56, 24, 50, 18, 58, 26, 12, 44,
          4, 36, 14, 46, 6, 38, 60, 28, 52, 20, 62, 30, 54, 22, 3, 35, 11, 43,
          1, 33, 9, 41, 51, 19, 59, 27, 49, 17, 57, 25, 15, 47, 7, 39, 13, 45,
          5, 37, 63, 31, 55, 23, 61, 29, 53, 21);

float indexValue() {
  int x = int(mod(gl_FragCoord.x, 8));
  int y = int(mod(gl_FragCoord.y, 8));
  return indexMatrix8x8[(x + y * 8)] / 64.0;
}

float colDistance(vec3 col1, vec3 col2, float noise) {
  col1.x *= 3.14159265356 * 2.0;
  col2.x *= 3.14159265356 * 2.0;

  float x =
      pow((sin(col1.x) * col1.y * col1.z - sin(col2.x) * col2.y * col2.z), 2);
  float y =
      pow((cos(col1.x) * col1.y * col1.z - cos(col2.x) * col2.y * col2.z), 2);
  float z = pow((col1.z - col2.z), 2);
  float dist = sqrt(x + y + z);

  if (noise < 0.25) { //(0.5+(0.5*sin(iTime*0.50)))) {
    dist = abs(col1.z - col2.z);
  }

  return dist;
}

vec3[2] closestColors(vec3 col, float noise) {
  vec3 palette[16] = vec3[](vec3(0.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0),
                            vec3(255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0),
                            vec3(104.0 / 255.0, 55.0 / 255.0, 43.0 / 255.0),
                            vec3(112.0 / 255.0, 164.0 / 255.0, 178.0 / 255.0),
                            vec3(111.0 / 255.0, 61.0 / 255.0, 133.6 / 255.0),
                            vec3(88.0 / 255.0, 141.0 / 255.0, 67.0 / 255.0),
                            vec3(53.0 / 255.0, 40.0 / 255.0, 121.0 / 255.0),
                            vec3(184.0 / 255.0, 199.0 / 255.0, 111.0 / 255.0),
                            vec3(111.0 / 255.0, 79.0 / 255.0, 37.0 / 255.0),
                            vec3(67.0 / 255.0, 57.0 / 255.0, 0.0 / 255.0),
                            vec3(154.0 / 255.0, 103.0 / 255.0, 89.0 / 255.0),
                            vec3(68.0 / 255.0, 68.0 / 255.0, 68.0 / 255.0),
                            vec3(108.0 / 255.0, 108.0 / 255.0, 108.0 / 255.0),
                            vec3(154.0 / 255.0, 210.0 / 255.0, 132.0 / 255.0),
                            vec3(108.0 / 255.0, 94.0 / 255.0, 181.0 / 255.0),
                            vec3(149.0 / 255.0, 149.0 / 255.0, 149.0 / 255.0));

  vec3 ret[2];
  vec3 closest = vec3(-2, 0, 0);
  vec3 secondClosest = vec3(-2, 0, 0);
  vec3 temp;
  for (int i = 0; i < 16; ++i) {
    temp = rgb2hsv(palette[i]);
    float tempDistance = colDistance(temp, col, noise);
    if (tempDistance < colDistance(closest, col, noise)) {
      secondClosest = closest;
      closest = temp;
    } else {
      if (tempDistance < colDistance(secondClosest, col, noise)) {
        secondClosest = temp;
      }
    }
  }
  ret[0] = closest;
  ret[1] = secondClosest;
  return ret;
}

vec3 dither(vec3 color, float noise) {
  vec3 hslCol = rgb2hsv(color);

  vec3 cs[2] = closestColors(hslCol, noise);
  vec3 c1 = cs[0];
  vec3 c2 = cs[1];
  float d = indexValue();
  float diff =
      colDistance(hslCol, c1, noise) /
      (colDistance(hslCol, c1, noise) + colDistance(hslCol, c2, noise));

  vec3 resultColor = (diff < d) ? c1 : c2;
  return hsv2rgb(resultColor);
}

void main() {
  // vec2 txc = TexCoord;
  // vec2 uv = txc * 2. - 1.;
  // vec3 txc_time = vec3(txc.x + 0.1 * sin(-iTime*0.3), txc.y + 0.2 *
  // cos(iTime*0.4), 14.6 * sin(iTime * 0.012));

  // float sim_noise = noise(vec3(txc_time * 1.3) + 23.5);

  vec2 flippedUV = vec2(TexCoord.x, 1.0 - TexCoord.y);
  vec3 sourceCol = texture(texture1, flippedUV).rgb;
  vec3 uv = vec3(TexCoord.xy, 0);
  vec3 noise = random3(uv);
  // float lum = dot(sourceCol, weight);
  // vec4 greyScale = vec4(lum, lum, lum,1);
  // FragColor = greyScale;
  FragColor = vec4(dither(sourceCol, noise.x), 1);
}