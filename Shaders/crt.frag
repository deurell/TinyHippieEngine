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
uniform float crtBrightness;
uniform float iTime;

out vec4 FragColor;

vec2 curve(vec2 uv) {
    float aspect = screenSize.x / max(screenSize.y, 1.0);
    vec2 p = (uv - 0.5) * 2.0;
    p.x *= aspect;

    // Changed: curvature is now an amount, not a multiplier.
    // 0.0 = flat, 1.0 = original-ish curve.
    float c = crtCurvature;

    vec2 q = p;
    q.x *= 1.0 + c * pow(abs(p.y), 2.0) * 0.04;
    q.y *= 1.0 + c * pow(abs(p.x), 2.0) * 0.04;

    q.x /= aspect;
    q = q * 0.5 + 0.5;
    q = q * 0.92 + 0.04;
    return q;
}

void main() {
    vec2 q = TexCoord;
    vec2 uv = curve(q);

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 texel = screenSize.x > 0.0 && screenSize.y > 0.0
    ? vec2(1.0) / screenSize
    : vec2(0.0);

    float wobble =
    sin(0.3 * iTime + uv.y * 21.0) *
    sin(0.7 * iTime + uv.y * 29.0) *
    sin(0.3 + 0.33 * iTime + uv.y * 31.0) *
    crtWobbleStrength;

    vec3 color;

    color.r = texture(texture0, vec2(
            uv.x + wobble + texel.x * 1.2,
            uv.y + texel.y * 0.8
    )).r + 0.03;

    color.g = texture(texture0, vec2(
            uv.x + wobble,
            uv.y - texel.y * 1.5
    )).g + 0.03;

    color.b = texture(texture0, vec2(
            uv.x + wobble - texel.x * 1.6,
            uv.y
    )).b + 0.03;

    // Changed: fixed UV offsets replaced by texel offsets.
    // This keeps the same idea but prevents resolution-dependent smearing.
    color.r += 0.06 * texture(texture0, uv + texel * vec2(8.0, -10.0)).r;
    color.g += 0.04 * texture(texture0, uv + texel * vec2(-7.0, -8.0)).g;
    color.b += 0.06 * texture(texture0, uv + texel * vec2(-8.0, -7.0)).b;

    color = clamp(color * 0.62 + 0.38 * color * color, 0.0, 1.0);

    float vig = 16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
    color *= vec3(pow(clamp(vig, 0.0, 1.0), 0.22 + crtVignetteStrength * 0.18));

    color *= vec3(0.95, 1.03, 0.95);
    color *= crtBrightness;

    float scans = clamp(
            0.35 + 0.35 * sin(uv.y * screenSize.y * 1.5),
            0.0,
            1.0
    );

    float scanline = pow(scans, mix(2.2, 1.3, crtScanlineStrength));

    color *= vec3(
            mix(0.72, 0.40, crtScanlineStrength) +
            mix(0.28, 0.78, crtScanlineStrength) * scanline
    );

    // Changed: softer 2-pixel mask instead of hard 3-column grille.
    // This preserves CRT texture but reduces the dot/mesh look.
    float grille = 1.0 - crtGrilleStrength * 0.12 *
    clamp((mod(gl_FragCoord.x, 2.0) - 1.0) * 2.0, 0.0, 1.0);

    color *= grille;

    FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
