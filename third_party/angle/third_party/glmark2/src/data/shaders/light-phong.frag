varying vec3 vertex_normal;
varying vec4 vertex_position;

vec4
compute_color(vec4 light_position, vec4 diffuse_light_color)
{
    const vec4 lightAmbient = vec4(0.1, 0.1, 0.1, 1.0);
    const vec4 lightSpecular = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 matAmbient = vec4(0.2, 0.2, 0.2, 1.0);
    const vec4 matSpecular = vec4(1.0, 1.0, 1.0, 1.0);
    const float matShininess = 100.0;
    vec3 eye_direction = normalize(-vertex_position.xyz);
    vec3 light_direction = normalize(light_position.xyz/light_position.w -
                                     vertex_position.xyz/vertex_position.w);
    vec3 normalized_normal = normalize(vertex_normal);
    vec3 reflection = reflect(-light_direction, normalized_normal);
    float specularTerm = pow(max(0.0, dot(reflection, eye_direction)), matShininess);
    float diffuseTerm = max(0.0, dot(normalized_normal, light_direction));
    vec4 specular = (lightSpecular * matSpecular);
    vec4 ambient = (lightAmbient * matAmbient);
    vec4 diffuse = (diffuse_light_color * MaterialDiffuse);
    vec4 result = (specular * specularTerm) + ambient + (diffuse * diffuseTerm);
    return result;
}

void main(void)
{
$DO_LIGHTS$
}
