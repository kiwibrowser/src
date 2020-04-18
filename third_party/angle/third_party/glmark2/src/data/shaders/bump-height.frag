#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D HeightMap;

varying vec2 TextureCoord;
varying vec3 NormalEye;
varying vec3 TangentEye;
varying vec3 BitangentEye;

void main(void)
{
    const vec4 LightSourceAmbient = vec4(0.1, 0.1, 0.1, 1.0);
    const vec4 LightSourceDiffuse = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 LightSourceSpecular = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 MaterialAmbient = vec4(1.0, 1.0, 1.0, 1.0);
    const vec4 MaterialDiffuse = vec4(1.0, 1.0, 1.0, 1.0);
    const vec4 MaterialSpecular = vec4(0.2, 0.2, 0.2, 1.0);
    const float MaterialShininess = 100.0;
    const float height_factor = 13.0;

    // Get the data from the height map
    float height0 = texture2D(HeightMap, TextureCoord).x;
    float heightX = texture2D(HeightMap, TextureCoord + vec2(TextureStepX, 0.0)).x;
    float heightY = texture2D(HeightMap, TextureCoord + vec2(0.0, TextureStepY)).x;
    vec2 dh = vec2(heightX - height0, heightY - height0);

    // Adjust the normal based on the height map data
    vec3 N = NormalEye - height_factor * dh.x * TangentEye -
                         height_factor * dh.y * BitangentEye;
    N = normalize(N);

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

    //gl_FragColor = vec4(height_diff_raw.xy, 0.0, 1.0);
    //gl_FragColor = vec4(height_diff_scaled.xy, 0.0, 1.0);
    //gl_FragColor = vec4(Tangent, 1.0);
}
