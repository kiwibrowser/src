varying vec3 vertex_normal;
varying vec4 vertex_position;

void main(void)
{
    const vec4 OutlineColor = vec4(0.0, 0.0, 0.0, 1.0);
    const vec2 OutlineThickness = vec2(0.1, 0.4);
    const vec4 BaseColor = vec4(0.0, 0.3, 0.0, 1.0);
    const vec4 LightColor = vec4(1.0, 1.0, 1.0, 1.0);
    const vec4 LightSourcePosition = vec4(4.0, 3.0, 1.0, 1.0);
    const vec4 DiffuseColor = vec4(0.0, 0.6, 0.0, 1.0);
    const vec4 SpecularColor = vec4(1.0, 1.0, 1.0, 0.7);
    const float DiffuseThreshold = 0.1;
    const float SpecularThreshold = 0.5;
    const float Shininess = 10.0;

    // Initialize the fragment color with an unlit value.
    vec4 fragColor = BaseColor;

    // Set up factors for computing diffuse illumination
    vec3 vertex_light = LightSourcePosition.xyz - vertex_position.xyz;
    vec3 N = normalize(vertex_normal);
    vec3 L = normalize(vertex_light);
    float NdotL = dot(N, L);
    float maxNdotL = max(NdotL, 0.0);
    float attenuation = length(LightSourcePosition) / length(vertex_light);

    // See if we have a diffuse contribution...
    // This will only be true if the interpolated normal and the light
    // are pointing in the "same" direction, and the attenuation due to
    // distance allows enough light for diffuse reflection.
    if (attenuation * maxNdotL >= DiffuseThreshold) {
        fragColor = LightColor * DiffuseColor;
    }

    // See if this fragment is part of the silhouette
    // If it is facing away from the viewer enough not to get any
    // diffuse illumination contribution, then it is close enough
    // to the silouhette to be painted with the outline color rather
    // than the unlit color.
    vec3 V = normalize(-vertex_position.xyz);
    if (dot(V, N) <
        mix(OutlineThickness.x, OutlineThickness.y, maxNdotL)) {
        fragColor = LightColor * OutlineColor;
    }

    // See if we have a specular contribution...
    // If the interpolated normal direction and the light direction
    // are facing the "same" direction, and the attenuated specular
    // intensity is strong enough, then we have a contribution.
    vec3 R = reflect(-L, N);
    float specularIntensity = pow(max(0.0, dot(R, V)), Shininess);
    if (NdotL > 0.0 && attenuation * specularIntensity > SpecularThreshold) {
        fragColor = SpecularColor.a * LightColor * SpecularColor +
            (1.0 - SpecularColor.a) * fragColor;
    }

    // Emit the final color
    gl_FragColor = vec4(fragColor.xyz, 1.0);
}
