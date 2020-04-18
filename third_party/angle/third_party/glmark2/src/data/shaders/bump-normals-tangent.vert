attribute vec3 position;
attribute vec2 texcoord;
attribute vec3 normal;
attribute vec3 tangent;

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 NormalMatrix;

varying vec2 TextureCoord;
varying mat4 TangentToEyeMatrix;

void main(void)
{
    TextureCoord = texcoord;

    // Calculate bitangent
    vec3 bitangent = normalize(cross(normal, tangent));

    // Calculate tangent space to eye space transformation matrix.
    // We need this matrix in the fragment shader to transform the data
    // from the normal map, which is in tangent space, to eye space,
    // so that we can perform meaningful lighting calculations. Alternatively,
    // we could have used an EyeToTangentMatrix, to convert light direction etc
    // to tangent space.

    // First calculate a tangent space to object space transformation matrix.
    mat4 TangentToObject = mat4(tangent.x, tangent.y, tangent.z, 0.0,
                                bitangent.x, bitangent.y, bitangent.z, 0.0,
                                normal.x, normal.y, normal.z, 0.0,
                                0.0, 0.0, 0.0, 1.0);

    // Multiply with the NormalMatrix to further transform from object
    // space to eye space (we are manipulating normals, so we can't use
    // the ModelView matrix for this step!).
    TangentToEyeMatrix = NormalMatrix * TangentToObject;

    // Transform the position to clip coordinates
    gl_Position = ModelViewProjectionMatrix * vec4(position, 1.0);
}
