#ifdef GL_ES
precision highp float;
precision highp int;
#endif

in vec2 TexCoord;

uniform sampler2D texture0;
uniform vec4 bump1;
uniform vec4 bump2;

out vec4 FragColor;

vec2 bumpOffset(vec2 uv, vec4 bump) {
    vec2 center = bump.xy;
    float age = clamp(bump.z, 0.0, 1.0);
    float strength = bump.w;
    vec2 delta = uv - center;
    float dist = length(delta);
    float radius = mix(0.03, 0.88, age);
    float softWidth = mix(0.31, 0.23, age);
    float wave = 1.0 - smoothstep(0.0, softWidth, abs(dist - radius));
    wave = wave * wave * (3.0 - 2.0 * wave);
    float fade = (1.0 - age) * (1.0 - age) * (1.0 - age * 0.35);
    vec2 dir = delta / max(dist, 0.0001);
    return dir * wave * fade * strength * 0.52;
}

float bumpWave(vec2 uv, vec4 bump) {
    vec2 center = bump.xy;
    float age = clamp(bump.z, 0.0, 1.0);
    float dist = length(uv - center);
    float radius = mix(0.03, 0.88, age);
    float glow = 1.0 - smoothstep(0.0, mix(0.36, 0.25, age), abs(dist - radius));
    float centerFlash = 1.0 - smoothstep(0.0, mix(0.34, 0.12, age), dist);
    float fade = (1.0 - age) * (1.0 - age) * (1.0 - age * 0.35);
    return (glow * 1.10 + centerFlash * 0.20) * fade * bump.w;
}

void main() {
    vec2 uv = TexCoord;
    vec2 offset = bumpOffset(uv, bump1) + bumpOffset(uv, bump2);
    float wave = bumpWave(uv, bump1) + bumpWave(uv, bump2);

    vec2 sampleUv = clamp(uv - offset, vec2(0.001), vec2(0.999));
    vec4 color = texture(texture0, sampleUv);

    float bloom = min(wave * 8.0 + length(offset) * 4.0, 0.34);
    float contrast = min(wave * 1.10, 0.12);
    float edge = 1.0 - smoothstep(
        0.0,
        0.075,
        min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y)));
    float heat = min((bump1.w + bump2.w) * 10.0, 1.0);

    color.rgb = (color.rgb - vec3(0.5)) * (1.0 + contrast) + vec3(0.5);
    color.rgb += vec3(0.42, 0.88, 1.0) * bloom;
    color.rgb += mix(vec3(0.92, 0.42, 0.92), vec3(1.0, 0.74, 0.40), heat) *
                 min(wave * 1.05, 0.09 + heat * 0.05);
    color.rgb += vec3(0.32, 0.84, 1.0) * edge * min(wave * 4.0, 0.16);

    FragColor = vec4(color.rgb, 1.0);
}
