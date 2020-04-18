uniform sampler2D ShadowMap;

varying vec4 Color;
varying vec4 ShadowCoord;

void main()
{
    vec4 sc_perspective = ShadowCoord / ShadowCoord.w;
    sc_perspective.z += 0.1505;
    vec4 shadow_value = texture2D(ShadowMap, sc_perspective.st);
    float light_distance = shadow_value.z;
    float shadow = 1.0;
    if (ShadowCoord.w > 0.0 && light_distance < sc_perspective.z) {
        shadow = 0.5;
    }
    gl_FragColor = vec4(shadow * Color.rgb, 1.0);
}
