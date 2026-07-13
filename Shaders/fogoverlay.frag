#ifdef GL_ES
precision mediump float;
precision mediump int;
#endif

in vec2 TexCoord;

uniform float iTime;
uniform sampler2D texture0;
uniform vec4 fogColor;
uniform vec2 tiling;
uniform vec2 scrollSpeed;
uniform float alpha;
uniform float softness;
uniform float secondLayerStrength;
uniform vec2 secondLayerScrollSpeed;
uniform float pulseAmount;
uniform float pulseSpeed;

out vec4 FragColor;

void main() {
  vec2 baseUv = TexCoord * max(tiling, vec2(0.001));
  float layerA = texture(texture0, fract(baseUv + scrollSpeed * iTime)).a;
  float layerB = texture(texture0, fract(baseUv * 1.63 +
                                         secondLayerScrollSpeed * iTime +
                                         vec2(0.37, 0.19))).a;
  float mist = mix(layerA, max(layerA, layerB), clamp(secondLayerStrength, 0.0, 1.0));
  float softnessValue = clamp(softness, 0.0, 1.0);
  float densityCurve = mix(1.65, 0.55, softnessValue);
  mist = pow(clamp(mist, 0.0, 1.0), densityCurve);

  vec2 edgeDistance = min(TexCoord, vec2(1.0) - TexCoord);
  float edgeFade = smoothstep(0.0, 0.16, min(edgeDistance.x, edgeDistance.y));
  float pulse = 1.0 + sin(iTime * pulseSpeed) * clamp(pulseAmount, 0.0, 1.0);
  float outAlpha = mist * edgeFade * alpha * pulse;
  FragColor = vec4(fogColor.rgb, outAlpha);
}
