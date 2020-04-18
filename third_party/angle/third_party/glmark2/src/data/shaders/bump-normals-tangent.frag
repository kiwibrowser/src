#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D NormalMap;

varying vec2 TextureCoord;
varying mat4 TangentToEyeMatrix;

void main(void)
{
    const vec4 LightSourceAmbient = vec4(0.1, 0.1, 0.1, 1.0);
    const vec4 LightSourceDiffuse = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 LightSourceSpecular = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 MaterialAmbient = vec4(1.0, 1.0, 1.0, 1.0);
    const vec4 MaterialDiffuse = vec4(1.0, 1.0, 1.0, 1.0);
    const vec4 MaterialSpecular = vec4(0.2, 0.2, 0.2, 1.0);
    const float MaterialShininess = 100.0;

    // Get the raw normal XYZ data from the normal map
    vec3 normal_raw = texture2D(NormalMap, TextureCoord).xyz;

    // Map "color" range [0, 1.0] to normal range [-1.0, 1.0]
    vec3 normal_scaled = normal_raw * 2.0 - 1.0;

    // The normal data is in tangent space, convert it to eye space so that
    // lighting calculations can work (light information is in eye space).
    vec3 N = normalize(vec3(TangentToEyeMatrix * vec4(normal_scaled, 1.0)));

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
    gl_FragColor = ambient + specular + diffuse;
    //gl_FragColor = vec4(N, 1.0);
}
