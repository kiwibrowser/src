varying vec3 Normal;

void main(void)
{
    const vec4 LightSourceAmbient = vec4(0.1, 0.1, 0.1, 1.0);
    const vec4 LightSourceDiffuse = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 LightSourceSpecular = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 MaterialAmbient = vec4(1.0, 1.0, 1.0, 1.0);
    const vec4 MaterialDiffuse = vec4(0.0, 0.0, 1.0, 1.0);
    const vec4 MaterialSpecular = vec4(1.0, 1.0, 1.0, 1.0);
    const float MaterialShininess = 100.0;

    vec3 N = normalize(Normal);

    // In the lighting model we are using here (Blinn-Phong with light at
    // infinity, viewer at infinity), the light position/direction and the
    // half vector is constant for the all the fragments.
    vec3 L = normalize(LightSourcePosition.xyz);
    vec3 H = normalize(LightSourceHalfVector);

    // Calculate the diffuse color according to Lambertian reflectance
    vec4 diffuse = MaterialDiffuse * LightSourceDiffuse * max(dot(N, L), 0.0);

    // Calculate the ambient color
    vec4 ambient = MaterialAmbient * LightSourceAmbient;

    // Calculate the specular color according to the Blinn-Phong model
    vec4 specular = MaterialSpecular * LightSourceSpecular *
                    pow(max(dot(N,H), 0.0), MaterialShininess);

    // Calculate the final color
    gl_FragColor = vec4((ambient + specular + diffuse).xyz, 1.0);
}
