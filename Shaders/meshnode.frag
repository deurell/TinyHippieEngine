precision highp float;
precision highp int;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture0;
uniform vec3 viewPos;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;
uniform vec3 baseTint;
uniform int debugNormals;

out vec4 FragColor;

void main() {
  vec3 norm = normalize(Normal);
  if (debugNormals != 0) {
    FragColor = vec4(norm * 0.5 + 0.5, 1.0);
    return;
  }
  vec3 lightDir = normalize(lightDirection);
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);

  vec2 invertedTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
  vec4 texel = texture(texture0, invertedTexCoord);
  vec3 albedo = texel.rgb * baseTint;

  float ndotl = dot(norm, lightDir);
  float diffuseTerm = clamp(ndotl * 0.5 + 0.5, 0.0, 1.0);
  float specularTerm = 0.0;
  if (ndotl > 0.0) {
    specularTerm = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
  }

  float skyFactor = clamp(norm.y * 0.5 + 0.5, 0.0, 1.0);
  vec3 hemiAmbient = mix(vec3(0.18, 0.17, 0.16), vec3(0.5, 0.52, 0.56), skyFactor);
  vec3 ambient = albedo * hemiAmbient * ambientStrength;
  vec3 diffuse = albedo * diffuseTerm * lightColor;
  vec3 specular = lightColor * specularStrength * specularTerm;

  FragColor = vec4(ambient + diffuse + specular, texel.a);
}
