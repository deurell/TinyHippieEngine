precision mediump float;
precision mediump int;

in vec2 TexCoord;
uniform vec4 baseColor;
uniform vec4 hotColor;
uniform vec4 deepColor;
uniform float paletteSteps;
uniform float coreRadius;
uniform float haloRadius;
uniform float outerRadius;
uniform float sparkleAmount;
out vec4 FragColor;

void main() {
    vec2 uv = TexCoord * 2.0 - 1.0;
    float dist = length(uv);
    if (dist > outerRadius) {
        discard;
    }

    float normalized = 1.0 - clamp(dist / max(outerRadius, 0.0001), 0.0, 1.0);
    float stepped = floor(normalized * max(paletteSteps, 1.0)) / max(paletteSteps, 1.0);
    float core = dist < coreRadius ? 1.0 : 0.0;
    float halo = dist < haloRadius ? 0.75 : 0.35;
    float sparkleMask = mod(floor((uv.x + uv.y + 2.0) * 6.0), 2.0) == 0.0 ? 1.0 : (1.0 - sparkleAmount);
    float sparkle = mix(1.0, sparkleMask, clamp(sparkleAmount, 0.0, 1.0));
    float intensity = max(stepped * halo, core) * sparkle;
    float alpha = intensity;
    vec3 color = mix(deepColor.rgb, baseColor.rgb, stepped);
    color = mix(color, hotColor.rgb, core);
    color *= 0.85 + intensity * 0.55;
    alpha *= 0.82;
    FragColor = vec4(color, alpha);
}
