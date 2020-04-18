attribute vec3 position;
attribute vec3 normal;

uniform mat4 ModelViewProjectionMatrix;

varying vec3 Normal;

void main(void)
{
    Normal = normal;
    gl_Position = ModelViewProjectionMatrix * vec4(position, 1.0);
}
