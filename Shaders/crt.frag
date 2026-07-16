#ifdef GL_ES
precision highp float;
precision highp int;
#endif

in vec2 TexCoord;

uniform sampler2D texture0;
uniform vec2 screenSize;
uniform float crtScanlineStrength;
uniform float crtVignetteStrength;
uniform float crtCurvature;
uniform float crtWobbleStrength;
uniform float crtGrilleStrength;
uniform float crtChromaticStrength;
uniform float crtBrightness;
uniform float iTime;

out vec4 FragColor;

float saturate(float value) {
    return clamp(value, 0.0, 1.0);
}

vec2 saturate(vec2 value) {
    return clamp(value, vec2(0.0), vec2(1.0));
}

vec3 saturate(vec3 value) {
    return clamp(value, vec3(0.0), vec3(1.0));
}

vec2 curveUv(vec2 uv, vec2 textureSizePixels) {
    float curvature = saturate(crtCurvature);
    if (curvature <= 0.0) {
        return uv;
    }

    float aspect = textureSizePixels.x / max(textureSizePixels.y, 1.0);
    vec2 p = (uv - 0.5) * 2.0;
    p.x *= aspect;

    vec2 curved = p;
    curved.x *= 1.0 + pow(abs(p.y), 2.0) * 0.045;
    curved.y *= 1.0 + pow(abs(p.x), 2.0) * 0.045;
    curved.x /= aspect;
    curved = curved * 0.5 + 0.5;

    vec2 curvedInset = curved * 0.94 + 0.03;
    return mix(uv, curvedInset, curvature);
}

float scanlineMask(vec2 sourcePixels) {
    float strength = saturate(crtScanlineStrength);
    if (strength <= 0.0) {
        return 1.0;
    }

    float period = 4.0;
    float phase = sourcePixels.y / period;
    float wave = 0.5 + 0.5 * cos(phase * 6.28318530718);
    float softLine = smoothstep(0.10, 0.92, wave);
    float mask = mix(1.0, mix(0.80, 1.0, softLine), strength);
    return min(mask, 1.0);
}

float grilleMask(vec2 sourcePixels) {
    float strength = saturate(crtGrilleStrength);
    if (strength <= 0.0) {
        return 1.0;
    }

    float period = 3.0;
    float slot = mod(sourcePixels.x, period);
    float column = smoothstep(0.15, 0.75, slot) *
                   (1.0 - smoothstep(2.20, 2.85, slot));
    float mask = mix(0.92, 1.0, column);
    return mix(1.0, mask, strength);
}

float vignetteMask(vec2 uv) {
    float strength = saturate(crtVignetteStrength);
    if (strength <= 0.0) {
        return 1.0;
    }

    float vig = 16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
    float shaped = pow(saturate(vig), 0.28);
    return mix(1.0, shaped, strength);
}

void main() {
    vec2 textureSizePixels = vec2(textureSize(texture0, 0));
    vec2 texel = vec2(1.0) / max(textureSizePixels, vec2(1.0));
    vec2 uv = curveUv(TexCoord, textureSizePixels);

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float wobble =
        sin(0.3 * iTime + uv.y * 21.0) *
        sin(0.7 * iTime + uv.y * 29.0) *
        sin(0.3 + 0.33 * iTime + uv.y * 31.0) *
        crtWobbleStrength;

    vec2 sampleUv = saturate(vec2(uv.x + wobble, uv.y));
    float chromaticPixels = min(max(crtChromaticStrength, 0.0), 2.0);
    float chromaticScale = textureSizePixels.y / 720.0;
    vec2 chromaticOffset = vec2(chromaticPixels * chromaticScale * texel.x, 0.0);

    float red = texture(texture0, saturate(sampleUv + chromaticOffset)).r;
    float green = texture(texture0, sampleUv).g;
    float blue = texture(texture0, saturate(sampleUv - chromaticOffset)).b;
    vec3 color = vec3(red, green, blue);

    vec2 sourcePixels = sampleUv * textureSizePixels;
    vec2 maskPixels = sourcePixels * (720.0 / max(textureSizePixels.y, 1.0));
    color *= scanlineMask(maskPixels);
    color *= grilleMask(maskPixels);
    color *= vignetteMask(uv);
    color *= crtBrightness;

    FragColor = vec4(saturate(color), 1.0);
}
