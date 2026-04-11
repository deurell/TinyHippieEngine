precision mediump float;
precision mediump int;

in vec2 TexCoord;
uniform vec4 baseColor;
out vec4 FragColor;

void main() {
    vec2 uv = TexCoord * 2.0 - 1.0;
    float dist = length(uv);
    float core = smoothstep(0.45, 0.0, dist);
    float halo = smoothstep(1.0, 0.15, dist);
    float intensity = core + halo * 0.35;
    float alpha = clamp(intensity, 0.0, 1.0);
    vec3 color = baseColor.rgb * (0.4 + intensity * 1.6);
    FragColor = vec4(color, alpha);
}
