struct LightSourceParameters
{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 position;
};
LightSourceParameters lightSource[3];
uniform vec4 light0Position;
uniform vec4 light1Position;
uniform vec4 light2Position;
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
    lightSource[0] = LightSourceParameters(
        vec4(0.0, 0.0, 0.0, 1.0),
        vec4(1.0, 1.0, 1.0, 1.0),
        vec4(1.0, 1.0, 1.0, 1.0),
        vec4(0.0, 1.0, 0.0, 0.0)
    );
    lightSource[1] = LightSourceParameters(
        vec4(0.0, 0.0, 0.0, 1.0),
        vec4(0.3, 0.3, 0.5, 1.0),
        vec4(0.3, 0.3, 0.5, 1.0),
        vec4(-1.0, 0.0, 0.0, 0.0)
    );
    lightSource[2] = LightSourceParameters(
        vec4(0.2, 0.2, 0.2, 1.0),
        vec4(0.2, 0.2, 0.2, 1.0),
        vec4(0.2, 0.2, 0.2, 1.0),
        vec4(0.0, -1.0, 0.0, 0.0)
    );
    vec4 matAmbient = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 matDiffuse = vec4(1.0, 0.2, 0.2, 1.0);
    vec4 matSpecular = vec4(0.5, 0.5, 0.5, 1.0);
    float matShininess = 20.0;
    vec4 diffuseSum = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 specularSum = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 ambientSum = vec4(0.0, 0.0, 0.0, 0.0);
    vec3 normalized_normal = normalize(vertex_normal);
    lightSource[0].position = light0Position;
    lightSource[1].position = light1Position;
    lightSource[2].position = light2Position;
    for (int light = 0; light < 3; light++) {
        vec4 light_position = lightSource[light].position;
        vec3 light_direction = normalize(unitvec(vertex_position, light_position));
        vec3 reflection = reflect(-light_direction, normalized_normal);
        specularSum += pow(max(0.0, dot(reflection, eye_direction)), matShininess) * lightSource[light].specular;
        diffuseSum += max(0.0, dot(normalized_normal, light_direction)) * lightSource[light].diffuse;
        ambientSum += lightSource[light].ambient;
    }
    gl_FragColor = (matSpecular * specularSum) + (matAmbient * ambientSum) + (matDiffuse * diffuseSum);
}
