#ifdef GL_ES
precision highp float;
precision highp int;
#endif

in vec2 TexCoord;

uniform sampler2D texture0;
uniform vec4 bump1;
uniform vec4 bump2;
uniform vec2 screenSize;

out vec4 FragColor;

vec3 bumpSample(vec2 uv, vec4 bump) {
    vec2 center = bump.xy;
    float age = clamp(bump.z, 0.0, 1.0);
    float strength = bump.w;
    if (strength <= 0.0 || age >= 1.0) {
        return vec3(0.0);
    }

    float aspect = screenSize.x / max(screenSize.y, 1.0);
    vec2 delta = uv - center;
    vec2 radialDelta = vec2(delta.x * aspect, delta.y);
    float dist = length(radialDelta);
    float safeDist = max(dist, 0.00000001);

    float easedAge = age * age * (3.0 - 2.0 * age);
    float radius = mix(0.055, 0.94, easedAge);
    float softWidth = mix(0.38, 0.29, age);
    float shell = 1.0 - smoothstep(0.0, softWidth, abs(dist - radius));
    shell = shell * shell * (3.0 - 2.0 * shell);

    float fade = pow(1.0 - age, 2.35);
    float innerGlow = 1.0 - smoothstep(0.0, mix(0.38, 0.15, age), dist);
    float wave = (shell * 0.94 + innerGlow * 0.14) * fade * strength;

    vec2 direction = radialDelta / safeDist;
    vec2 uvDirection = vec2(direction.x / aspect, direction.y);
    float displacement = shell * fade * strength * 0.50;
    return vec3(uvDirection * displacement, wave);
}

void main() {
    vec2 uv = TexCoord;
    vec3 bumpA = bumpSample(uv, bump1);
    vec3 bumpB = bumpSample(uv, bump2);
    vec2 offset = bumpA.xy + bumpB.xy;
    float wave = bumpA.z + bumpB.z;

    vec2 sampleUv = clamp(uv - offset, vec2(0.001), vec2(0.999));
    vec4 color = texture(texture0, sampleUv);

    float bloom = min(wave * 5.8 + length(offset) * 2.8, 0.285);
    float contrast = min(wave * 0.74, 0.09);
    float edge = 1.0 - smoothstep(
        0.0,
        0.075,
        min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y)));
    float heat = min((bump1.w + bump2.w) * 10.0, 1.0);

    color.rgb = (color.rgb - vec3(0.5)) * (1.0 + contrast) + vec3(0.5);
    color.rgb += vec3(0.42, 0.88, 1.0) * bloom;
    color.rgb += mix(vec3(0.92, 0.42, 0.92), vec3(1.0, 0.74, 0.40), heat) *
                 min(wave * 0.98, 0.085 + heat * 0.045);
    color.rgb += vec3(0.32, 0.84, 1.0) * edge * min(wave * 3.2, 0.14);

    FragColor = vec4(color.rgb, 1.0);
}
