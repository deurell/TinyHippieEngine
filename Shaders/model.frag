precision highp float;
precision highp int;

struct Material {
  sampler2D diffuse;
  vec3 diffuseFallback;
  vec3 ambientFallback;
  vec3 specular;
  float shininess;
  int id;
};

struct DirLight {
  vec3 direction;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

struct PointLight {
  vec3 position;
  float constant;
  float linear;
  float quadratic;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform float iTime;
uniform vec3 viewPos;
uniform Material material;
uniform DirLight dirLight;
#define POINT_LIGHTS 2
uniform PointLight pointLights[POINT_LIGHTS];
uniform sampler2D texture_diffuse1;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec2 uv);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,
                    vec2 uv);

void main() {
  vec3 norm = normalize(Normal);
  vec3 viewDir = normalize(viewPos - FragPos);
  vec2 uv = TexCoords;
  if (material.id == 4) {
    uv.x = TexCoords.x + sin(iTime * 0.18 * uv.x);
    uv.y = TexCoords.y + sin(iTime * 0.25 * uv.y);
  }

  vec3 result = CalcDirLight(dirLight, norm, viewDir, uv);

  for (int i = 0; i < POINT_LIGHTS; i++) {
    result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, uv);
  }

  FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec2 uv) {
  vec3 lightDir = normalize(-light.direction);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 ambient = (material.ambientFallback == vec3(0, 0, 0))
                     ? light.ambient * vec3(texture(material.diffuse, uv))
                     : light.ambient * material.ambientFallback;
  vec3 diffuse =
      (material.diffuseFallback == vec3(0, 0, 0))
          ? light.diffuse * diff * vec3(texture(material.diffuse, uv))
          : light.diffuse * diff * material.diffuseFallback;
  if (dot(normal, lightDir) < 0.0) {
    spec = 0.0;
  }
  vec3 specular = light.specular * (spec * material.specular);
  return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,
                    vec2 uv) {
  vec3 lightDir = normalize(light.position - fragPos);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 reflectDir = reflect(-lightDir, normal);

  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  float distance = length(light.position - fragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance +
                             light.quadratic * (distance * distance));
  vec3 ambient = (material.ambientFallback == vec3(0, 0, 0))
                     ? light.ambient * vec3(texture(material.diffuse, uv))
                     : light.ambient * material.ambientFallback;
  vec3 diffuse =
      (material.diffuseFallback == vec3(0, 0, 0))
          ? light.diffuse * diff * vec3(texture(material.diffuse, uv))
          : light.diffuse * diff * material.diffuseFallback;

  if (dot(normal, lightDir) < 0.0) {
    spec = 0.0;
  }
  vec3 specular = light.specular * (spec * material.specular);
  ambient *= attenuation;
  diffuse *= attenuation;
  specular *= attenuation;
  return (ambient + diffuse + specular);
}
