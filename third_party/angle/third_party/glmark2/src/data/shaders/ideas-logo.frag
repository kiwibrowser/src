uniform vec4 light0Position;
varying vec3 vertex_normal;
varying vec4 vertex_position;
varying vec3 eye_direction;

vec3 unitvec(vec4 v1, vec4 v2)
{
    if (v1.w == 0.0 && v2.w == 0.0)
        return vec3(v2 - v1);
    if (v1.w == 0.0)
        return vec3(-v1);
    if (v2.w == 0.0)
        return vec3(v2);
    return v2.xyz/v2.w - v1.xyz/v1.w;
}

void main()
{
    vec4 lightAmbient = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 lightDiffuse = vec4(1.0, 1.0, 1.0, 1.0);
    vec4 lightSpecular = vec4(1.0, 1.0, 1.0, 1.0);
    vec4 matAmbient = vec4(0.1, 0.1, 0.1, 1.0);
    vec4 matDiffuse = vec4(0.5, 0.4, 0.7, 1.0);
    vec4 matSpecular = vec4(1.0, 1.0, 1.0, 1.0);
    float matShininess = 30.0;
    vec3 light_direction = normalize(unitvec(vertex_position, light0Position));
    vec3 normalized_normal = normalize(vertex_normal);
    vec3 reflection = reflect(-light_direction, normalized_normal);
    float specularTerm = pow(max(0.0, dot(reflection, eye_direction)), matShininess);
    float diffuseTerm = max(0.0, dot(normalized_normal, light_direction));
    vec4 specular = (lightSpecular * matSpecular);
    vec4 ambient = (lightAmbient * matAmbient);
    vec4 diffuse = (lightDiffuse * matDiffuse);
    gl_FragColor = (specular * specularTerm) + ambient + (diffuse * diffuseTerm);
}
