attribute vec3 position;
attribute vec3 normal;

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 NormalMatrix;
uniform mat4 ModelViewMatrix;
uniform mat4 LightMatrix;

varying vec3 vertex_normal;
varying vec4 vertex_position;
varying vec4 MapCoord;

void main(void)
{
    vec4 current_position = vec4(position, 1.0);

    // Transform the normal to eye coordinates
    vertex_normal = normalize(vec3(NormalMatrix * vec4(normal, 1.0)));

    // Transform the current position to eye coordinates
    vertex_position = ModelViewMatrix * current_position;

    // Transform the current position for use as texture coordinates
    MapCoord = LightMatrix * current_position;

    // Transform the current position to clip coordinates
    gl_Position = ModelViewProjectionMatrix * current_position;
}
