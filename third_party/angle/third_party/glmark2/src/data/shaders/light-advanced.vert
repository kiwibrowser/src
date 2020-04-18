attribute vec3 position;
attribute vec3 normal;

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 NormalMatrix;

varying vec3 Normal;

void main(void)
{
    // Transform the normal to eye coordinates
    Normal = normalize(vec3(NormalMatrix * vec4(normal, 1.0)));

    // Transform the position to clip coordinates
    gl_Position = ModelViewProjectionMatrix * vec4(position, 1.0);
}
