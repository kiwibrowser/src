uniform sampler2D DistanceMap;
uniform sampler2D NormalMap;
uniform sampler2D ImageMap;

varying vec3 vertex_normal;
varying vec4 vertex_position;
varying vec4 MapCoord;

void main()
{
    const vec4 lightSpecular = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 matSpecular = vec4(1.0, 1.0, 1.0, 1.0);
    const float matShininess = 100.0;
    const vec2 point_five = vec2(0.5);
    // Need the normalized eye direction and surface normal vectors to
    // compute the transmitted vector through the "front" surface of the object.
    vec3 eye_direction = normalize(-vertex_position.xyz);
    vec3 normalized_normal = normalize(vertex_normal);
    vec3 front_refraction = refract(eye_direction, normalized_normal, RefractiveIndex);
    // Find our best distance approximation through the object so we can
    // project the transmitted vector to the back of the object to find
    // the exit point.
    vec3 mc_perspective = (MapCoord.xyz / MapCoord.w) + front_refraction;
    vec2 dcoord = mc_perspective.st * point_five + point_five;
    vec4 distance_value = texture2D(DistanceMap, dcoord);
    vec3 back_position = vertex_position.xyz + front_refraction * distance_value.z;
    // Use the exit point to index the map of back-side normals, and use the
    // back-side position and normal to find the transmitted vector out of the
    // object.
    vec2 normcoord = back_position.st * point_five + point_five;
    vec3 back_normal = texture2D(NormalMap, normcoord).xyz;
    vec3 back_refraction = refract(back_position, back_normal, 1.0/RefractiveIndex);
    // Use the transmitted vector from the exit point to determine where
    // the vector would intersect the environment (in this case a background
    // image.
    vec2 imagecoord = back_refraction.st * point_five + point_five;
    vec4 texel = texture2D(ImageMap, imagecoord);
    // Add in specular reflection, and we have our fragment value.
    vec3 light_direction = normalize(vertex_position.xyz/vertex_position.w -
                                     LightSourcePosition.xyz/LightSourcePosition.w);
    vec3 reflection = reflect(light_direction, normalized_normal);
    float specularTerm = pow(max(0.0, dot(reflection, eye_direction)), matShininess);
    vec4 specular = (lightSpecular * matSpecular);
    gl_FragColor = (specular * specularTerm) + texel;
}
