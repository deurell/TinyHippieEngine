precision highp float;
precision highp int;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 materialDiffuse;
uniform vec3 materialAmbient;
uniform vec3 materialSpecular;
uniform float materialShininess;

out vec4 FragColor;

void main() {
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(lightDirection);
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);

  float ndotl = max(dot(norm, lightDir), 0.0);
  float specularTerm = 0.0;
  if (ndotl > 0.0) {
    specularTerm = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
  }

  vec3 ambient = materialAmbient;
  vec3 diffuse = materialDiffuse * ndotl * lightColor;
  vec3 specular = materialSpecular * specularTerm * lightColor;

  FragColor = vec4(ambient + diffuse + specular, 1.0);
}
