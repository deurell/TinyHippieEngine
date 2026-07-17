precision mediump float;
precision mediump int;

in vec2 TexCoord;

uniform float iTime;
uniform vec4 baseColor;
uniform vec4 proceduralParams;
uniform int proceduralStyle;

out vec4 FragColor;

float ring(float d, float radius, float width) {
    return 1.0 - smoothstep(0.0, width, abs(d - radius));
}

float electroNoise(vec2 p, float t) {
    float n = 0.0;
    n += sin(p.x * 11.0 + t * 1.4) * 0.50;
    n += sin(p.x * 23.0 - t * 2.1 + sin(p.x * 3.0)) * 0.25;
    n += sin(p.x * 47.0 + t * 3.2 + p.y * 4.0) * 0.12;
    n += sin(p.x * 91.0 - t * 4.0) * 0.06;
    return n;
}

vec4 shadeBeam(vec2 uv) {
    vec2 p = uv * 2.0 - 1.0;
    float energy = clamp(proceduralParams.x, 0.0, 3.0);
    float energy01 = clamp(energy / 3.0, 0.0, 1.0);
    float beamTime = iTime * 4.0;
    float lengthScale = max(abs(proceduralParams.y), 0.0001);
    float referenceLength = max(abs(proceduralParams.z), 0.0001);
    float beamX = p.x * lengthScale / referenceLength;
    float edgeMatch = smoothstep(0.0, 0.045, uv.x) *
                      smoothstep(0.0, 0.045, 1.0 - uv.x);
    float width = 0.18 + energy * 0.026;
    float center = electroNoise(vec2(beamX, p.y), beamTime) * width * edgeMatch;
    float y = abs(p.y - center);
    float glow = clamp(1.0 - pow(max(y, 0.0001), 0.20 + energy * 0.010), 0.0, 1.0);
    float halo = 1.0 - smoothstep(0.0, 0.85 + energy * 0.065, y);
    float core = smoothstep(0.052 + energy * 0.012, 0.000, y);
    float innerCore = smoothstep(0.022 + energy * 0.006, 0.000, y);
    float filament = smoothstep(0.032 + energy * 0.003, 0.000, abs(p.y - center + sin(beamX * 13.0 + beamTime * 1.7) * (0.028 + energy * 0.003)));
    float sparks = pow(max(0.0, sin(beamX * 37.0 - beamTime * 2.4)), 18.0) *
                   smoothstep(0.18, 0.0, y);
    vec3 electric = vec3(0.55, 0.92, 1.0);
    vec3 plasma = vec3(1.0, 0.45, 0.92);
    vec3 hot = vec3(1.0, 0.92, 1.0);
    vec3 beamColor = mix(electric, plasma, energy01 * 0.42) * baseColor.rgb;
    float intensity = 1.0 + energy * 0.19;
    vec3 color = beamColor * glow * 1.20 * intensity + beamColor * halo * (0.34 + energy * 0.035) * intensity;
    color *= mix(vec3(1.0), color, 0.68);
    color += mix(vec3(1.0, 0.96, 1.0), hot, energy01 * 0.65) * core * (1.44 + energy * 0.18);
    color += vec3(1.0) * innerCore * (1.36 + energy * 0.24);
    color += beamColor * filament * (0.40 + energy * 0.14);
    color += vec3(1.0, 0.86, 1.0) * sparks * (0.55 + energy * 0.18);
    float alpha = clamp(glow * (0.55 + energy * 0.035) +
                        halo * (0.10 + energy * 0.018) +
                        core * (0.65 + energy * 0.044) +
                        filament * (0.20 + energy * 0.02) + sparks * 0.27,
                        0.0, 0.76 + energy * 0.035);
    return vec4(color, alpha);
}

vec4 shadeSource(vec2 uv) {
    vec2 p = uv * 2.0 - 1.0;
    float d = length(p);
    float t = proceduralParams.x;
    float kick = clamp(proceduralParams.y, 0.0, 1.0);
    float load = clamp(proceduralParams.z, 0.0, 1.0);
    float pulse = 0.5 + 0.5 * sin(t * 5.0);
    float slow = 0.5 + 0.5 * sin(t * 1.7);
    float wobble = sin(p.x * 10.0 + iTime * 2.2) *
                   sin(p.y * 9.0 - iTime * 1.9) * 0.020;
    float core = 1.0 - smoothstep(0.0, 0.24 + kick * 0.10, d + wobble);
    float hotCore = 1.0 - smoothstep(0.0, 0.105 + kick * 0.05, d);
    float innerRing = ring(d, 0.34 + slow * 0.045, 0.050 + kick * 0.018);
    float mainRing = ring(d, 0.58 + pulse * 0.08 + kick * 0.16, 0.075 + kick * 0.030);
    float outerRing = ring(d, 0.78 + slow * 0.040, 0.090);
    float halo = 1.0 - smoothstep(0.0, 1.02, d + wobble * 0.7);
    vec3 beam = vec3(0.55, 0.92, 1.0);
    vec3 magenta = vec3(1.0, 0.45, 0.92);
    vec3 color = beam * halo * (0.42 + pulse * 0.24 + kick * 0.30 + load * 0.32);
    color += vec3(1.0, 0.96, 1.0) * hotCore * (1.55 + kick * 0.85);
    color += beam * core * (0.55 + slow * 0.20 + load * 0.24);
    color += beam * mainRing * (1.25 + kick * 0.95 + load * 0.45);
    color += magenta * innerRing * (0.22 + kick * 0.18 + load * 0.18);
    color += beam * outerRing * (0.28 + slow * 0.20);
    float alpha = clamp(hotCore + core * 0.60 + mainRing * 0.70 +
                        innerRing * 0.28 + outerRing * 0.26 + halo * 0.32,
                        0.0, 1.0);
    return vec4(color, alpha);
}

vec4 shadeTarget(vec2 uv) {
    vec2 p = uv * 2.0 - 1.0;
    float d = length(p);
    float pulse = 0.5 + 0.5 * sin(proceduralParams.x * 2.2 + proceduralParams.y);
    float hit = clamp(proceduralParams.z, 0.0, 1.0);
    float alpha = 1.0 - smoothstep(0.72, 0.94, d);
    float core = 1.0 - smoothstep(0.0, 0.62, d);
    float rim = smoothstep(0.50, 0.82, d) * (1.0 - smoothstep(0.82, 0.94, d));
    float inner = 1.0 - smoothstep(0.0, 0.34 + pulse * 0.04, d);
    float whiteCore = 1.0 - smoothstep(0.0, 0.30 + hit * 0.20, d);
    float hotRing = ring(d, mix(0.18, 0.78, 1.0 - hit), 0.095);
    float chargeHalo = 1.0 - smoothstep(0.0, 0.98, d);
    vec3 color = vec3(0.78, 0.10, 0.04) * core;
    color += vec3(2.8, 0.34, 0.10) * core * core * (0.08 + pulse * 1.55);
    color += vec3(1.0, 0.62, 0.18) * rim;
    color += vec3(1.0, 0.92, 0.35) * inner * (0.16 + pulse * 0.28);
    color += vec3(1.0, 0.95, 0.78) * whiteCore * hit * 2.2;
    color += vec3(0.55, 0.92, 1.0) * hotRing * hit * 1.15;
    color += vec3(1.0, 0.42, 0.24) * chargeHalo * hit * 0.65;
    return vec4(color, clamp(alpha + hit * 0.45, 0.0, 1.0));
}

vec4 shadeBlocker(vec2 uv, bool reflective) {
    vec2 p = uv * 2.0 - 1.0;
    vec2 ap = abs(p);
    float box = 1.0 - smoothstep(0.86, 0.94, max(ap.x, ap.y));
    float border = max(smoothstep(0.72, 0.79, ap.x),
                       smoothstep(0.72, 0.79, ap.y)) * box;
    float inner = (1.0 - border) * box;
    float depth = 1.0 - smoothstep(0.0, 0.92, length(p * vec2(0.85, 1.1)));
    vec3 solid = vec3(0.10, 0.13, 0.17) * inner +
                 vec3(0.07, 0.10, 0.13) * depth * inner +
                 vec3(0.38, 0.46, 0.54) * border;
    vec3 mirror = vec3(0.10, 0.21, 0.24) * inner +
                  vec3(0.06, 0.13, 0.16) * depth * inner +
                  vec3(0.28, 0.74, 0.82) * border;
    vec3 color = reflective ? mirror : solid;
    float glow = proceduralParams.x * proceduralParams.x;
    float energy = clamp(proceduralParams.y, 0.0, 1.0);
    vec2 hitPoint = proceduralParams.zw;
    vec3 beamColor = mix(vec3(0.55, 0.92, 1.0), vec3(1.0, 0.45, 0.92), energy * 0.65);
    float hitDistance = length(p - hitPoint);
    vec2 hitDir = normalize(hitPoint + vec2(0.0001));
    vec2 fromHit = p - hitPoint;
    float alongHit = max(dot(fromHit, -hitDir), 0.0);
    float acrossHit = abs(fromHit.x * hitDir.y - fromHit.y * hitDir.x);
    float localBloom = (1.0 - smoothstep(0.0, 1.72, hitDistance)) * glow * box;
    float localCore = (1.0 - smoothstep(0.0, 0.48, hitDistance)) * glow * inner;
    float impactStreak = (1.0 - smoothstep(0.0, 1.45, alongHit)) *
                         (1.0 - smoothstep(0.0, 0.38, acrossHit)) * glow * inner;
    color += beamColor * glow * box * (0.34 + energy * 0.22);
    color += beamColor * localBloom * (1.45 + energy * 0.48);
    color += beamColor * impactStreak * (1.75 + energy * 0.52);
    color += vec3(1.0, 0.95, 1.0) * localCore * (2.35 + energy * 0.62);
    color += vec3(1.0) * border * glow * (0.22 + energy * 0.18);
    return vec4(color, box);
}

vec4 shadeReflector(vec2 uv, bool automatic) {
    vec2 p = uv * 2.0 - 1.0;
    float body = 1.0 - smoothstep(0.72, 0.92, abs(p.y));
    float bevel = smoothstep(0.82, 0.20, abs(p.x));
    float shine = smoothstep(0.16, 0.0, abs(p.y + p.x * 0.18 - 0.22));
    float glow = clamp(proceduralParams.x, 0.0, 1.0);
    float selected = clamp(proceduralParams.z, 0.0, 1.0);
    float energy = clamp(proceduralParams.w, 0.0, 1.0);
    vec3 manualBase = vec3(0.58, 0.86, 1.0);
    vec3 autoBase = vec3(1.0, 0.60, 0.28);
    vec3 base = automatic ? autoBase : manualBase;
    vec3 edge = automatic ? vec3(0.45, 0.20, 0.08) : vec3(0.12, 0.30, 0.45);
    vec3 color = mix(edge, base, bevel);
    color += vec3(0.36, 0.60, 0.68) * shine;
    vec3 beamColor = mix(vec3(0.42, 0.78, 0.95), vec3(0.86, 0.36, 0.78), energy * 0.55);
    color += beamColor * glow * body * (0.22 + energy * 0.22);
    color += beamColor * shine * glow * (0.18 + energy * 0.16);
    color += vec3(1.0) * selected * body * 0.45;
    color = min(color, vec3(1.15));
    return vec4(color, clamp(body + glow * 0.16 + selected * 0.25, 0.0, 1.0));
}

vec4 shadeSelection(vec2 uv) {
    vec2 p = uv * 2.0 - 1.0;
    vec2 ap = abs(p);
    float d = length(p);
    float t = proceduralParams.x;
    float flash = proceduralParams.y;
    float pulse = 0.5 + 0.5 * sin(t * 5.0);
    float edge = max(ap.x, ap.y);
    float cornerZone = smoothstep(0.44, 0.62, edge);
    float cornerMask = smoothstep(0.30, 0.50, min(ap.x, ap.y));
    float outerCorner = cornerZone * cornerMask * (1.0 - smoothstep(0.78, 0.93, edge));
    float bracketX = (1.0 - smoothstep(0.018, 0.060, abs(ap.x - 0.70))) *
                     smoothstep(0.40, 0.56, ap.y) *
                     (1.0 - smoothstep(0.74, 0.94, ap.y));
    float bracketY = (1.0 - smoothstep(0.018, 0.060, abs(ap.y - 0.70))) *
                     smoothstep(0.40, 0.56, ap.x) *
                     (1.0 - smoothstep(0.74, 0.94, ap.x));
    float brackets = max(bracketX, bracketY) * outerCorner;
    vec2 dir = normalize(p + vec2(0.0001));
    vec2 sweepDir = vec2(cos(iTime * 2.4), sin(iTime * 2.4));
    float sweep = 1.0 - smoothstep(0.0, 0.13, abs(dot(dir, sweepDir)));
    float arc = ring(d, 0.58, 0.045) * sweep;
    float innerRing = ring(d, 0.31 + flash * 0.045, 0.035);
    float popRing = ring(d, mix(0.34, 0.76, 1.0 - flash), 0.10 + flash * 0.05) * flash;
    float alpha = brackets * (0.70 + pulse * 0.18 + flash * 0.30) +
                  arc * (0.28 + flash * 0.32) + innerRing * (0.18 + flash * 0.35) +
                  popRing * 0.50;
    vec3 color = vec3(0.55, 0.92, 1.0) * (0.80 + pulse * 0.20);
    color = mix(color, vec3(1.0, 0.45, 0.92), flash * 0.28);
    color += vec3(1.0) * (flash * 0.30 + popRing * 0.24);
    return vec4(color * alpha, clamp(alpha, 0.0, 1.0));
}

float hash11(float n) {
    return fract(sin(n) * 43758.5453123);
}

vec4 shadeExplosion(vec2 uv) {
    vec2 p = uv * 2.0 - 1.0;
    float amount = clamp(proceduralParams.x, 0.0, 1.0);
    float energy = clamp(proceduralParams.y, 0.0, 1.0);
    float seed = proceduralParams.z;
    float seedA = hash11(seed + 1.7);
    float seedB = hash11(seed + 9.3);

    vec2 drift = vec2(seedA - 0.5, seedB - 0.5) * 0.14 * amount;
    vec2 p0 = p - drift;
    vec2 p1 = p + vec2(seedB - 0.5, 0.5 - seedA) * 0.18;
    vec2 p2 = p + vec2(0.5 - seedA, seedB - 0.5) * 0.24;
    vec2 p3 = p + vec2(seedA - 0.5, seedA + seedB - 1.0) * 0.16;
    float d = length(p0 * vec2(0.92 + seedA * 0.10, 1.08 - seedB * 0.10));
    float d1 = length(p1 * vec2(1.16 + seedA * 0.14, 0.82 + seedB * 0.12));
    float d2 = length(p2 * vec2(0.78 + seedB * 0.16, 1.18 + seedA * 0.10));
    float d3 = length(p3 * vec2(1.05 - seedB * 0.12, 0.90 + seedA * 0.16));

    float flash = 1.0 - smoothstep(0.0, 0.18, amount);
    float bloomPhase = 1.0 - smoothstep(0.12, 0.88, amount);
    float ringPhase = 1.0 - smoothstep(0.36, 1.0, amount);
    float afterglow = 1.0 - smoothstep(0.52, 1.0, amount);
    float hotCore = 1.0 - smoothstep(0.0, 0.15 + flash * 0.16, d);
    float core = 1.0 - smoothstep(0.02, 0.48 + amount * 0.36, d1);
    float warmBloom = 1.0 - smoothstep(0.0, 1.12 + energy * 0.08, d2);
    float lobeA = 1.0 - smoothstep(0.02, 0.68 + amount * 0.30, d1);
    float lobeB = 1.0 - smoothstep(0.02, 0.58 + amount * 0.28, d2);
    float lobeC = 1.0 - smoothstep(0.02, 0.50 + amount * 0.24, d3);
    float cloud = max(core * 0.86, max(lobeA * 0.70, max(lobeB * 0.58, lobeC * 0.46)));
    float mainRing = ring(d1, mix(0.22, 0.78, amount), 0.44 + amount * 0.12);
    float shockRing = ring(d2, mix(0.42, 0.94, amount), 0.34 + amount * 0.12);
    float lateGlow = 1.0 - smoothstep(0.0, 0.76 + seedA * 0.10, d1);
    float emberCloud = cloud * afterglow;

    vec3 beamCyan = vec3(0.55, 0.92, 1.0);
    vec3 targetWarm = vec3(1.35, 0.18, 0.05);
    vec3 flashWarm = vec3(1.0, 0.92, 0.66);
    vec3 ringColor = mix(beamCyan, vec3(1.0, 0.45, 0.92), clamp(energy * 0.34 + seedB * 0.08, 0.0, 0.48));
    vec3 emberColor = mix(targetWarm, vec3(1.0, 0.58, 0.18), seedA * 0.35);
    float variantHeat = 0.88 + seedA * 0.24;

    vec3 color = targetWarm * cloud * bloomPhase * 0.76 * variantHeat;
    color += flashWarm * hotCore * (bloomPhase * 0.46 + flash * 0.52);
    color += emberColor * warmBloom * bloomPhase * (0.18 + energy * 0.08);
    color += ringColor * mainRing * ringPhase * (0.16 + energy * 0.08);
    color += beamCyan * shockRing * ringPhase * (0.07 + energy * 0.04);
    color += emberColor * emberCloud * afterglow * 0.28;
    color += beamCyan * lateGlow * afterglow * 0.04;
    color += vec3(1.0, 0.92, 0.66) * hotCore * flash * 0.24;

    float alpha = clamp(cloud * bloomPhase * 0.62 + hotCore * flash * 0.54 +
                        mainRing * ringPhase * 0.10 +
                        shockRing * ringPhase * 0.04 +
                        warmBloom * afterglow * 0.14 +
                        emberCloud * 0.12,
                        0.0, 1.0);
    return vec4(color, alpha);
}

void main() {
    vec4 color = baseColor;
    if (proceduralStyle == 1) {
        color = shadeBeam(TexCoord);
    } else if (proceduralStyle == 2) {
        color = shadeSource(TexCoord);
    } else if (proceduralStyle == 3) {
        color = shadeTarget(TexCoord);
    } else if (proceduralStyle == 4) {
        color = shadeBlocker(TexCoord, false);
    } else if (proceduralStyle == 5) {
        color = shadeBlocker(TexCoord, true);
    } else if (proceduralStyle == 6) {
        color = shadeReflector(TexCoord, false);
    } else if (proceduralStyle == 7) {
        color = shadeReflector(TexCoord, true);
    } else if (proceduralStyle == 8) {
        color = shadeSelection(TexCoord);
    } else if (proceduralStyle == 9) {
        color = shadeExplosion(TexCoord);
    }

    color *= vec4(baseColor.rgb, 1.0);
    if (color.a < 0.025) {
        discard;
    }
    FragColor = color;
}
