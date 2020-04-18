attribute vec3 position;
attribute vec3 normal;
attribute vec2 texcoord;

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 NormalMatrix;

varying vec4 Color;
varying vec2 TextureCoord;

void main(void)
{
    // Transform the normal to eye coordinates
    vec3 N = normalize(vec3(NormalMatrix * vec4(normal, 1.0)));

    // The LightSourcePosition is actually its direction for directional light
    vec3 L = normalize(LightSourcePosition.xyz);

    // Multiply the diffuse value by the vertex color (which is fixed in this case)
    // to get the actual color that we will use to draw this vertex with
    float diffuse = max(dot(N, L), 0.0);
    Color = vec4(diffuse * MaterialDiffuse.rgb, MaterialDiffuse.a);

    // Set the texture coordinates as a varying
    TextureCoord = texcoord;

    // Transform the position to clip coordinates
    gl_Position = ModelViewProjectionMatrix * vec4(position, 1.0);
}
