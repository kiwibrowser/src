attribute vec3 position;
attribute vec2 texcoord;
attribute vec3 normal;
attribute vec3 tangent;

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 NormalMatrix;

varying vec2 TextureCoord;
varying vec3 NormalEye;
varying vec3 TangentEye;
varying vec3 BitangentEye;

void main(void)
{
    TextureCoord = texcoord;

    // Transform normal, tangent and bitangent to eye space, keeping
    // all of them perpendicular to the Normal. That is why we use
    // NormalMatrix, instead of ModelView, to transform the tangent and
    // bitangent.
    NormalEye = normalize(vec3(NormalMatrix * vec4(normal, 1.0)));
    TangentEye = normalize(vec3(NormalMatrix * vec4(tangent, 1.0)));
    BitangentEye = normalize(vec3(NormalMatrix * vec4(cross(normal, tangent), 1.0)));

    // Transform the position to clip coordinates
    gl_Position = ModelViewProjectionMatrix * vec4(position, 1.0);
}
