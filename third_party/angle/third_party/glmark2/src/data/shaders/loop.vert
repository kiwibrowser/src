attribute vec3 position;

uniform mat4 ModelViewProjectionMatrix;
uniform int VertexLoops;

// Removing this varying causes an inexplicable performance regression
// with r600g... Keeping it for now.
varying vec4 dummy;

void main(void)
{
    dummy = vec4(1.0);

    float d = fract(position.x);

$MAIN$

    vec4 pos = vec4(position.x,
                    position.y + 0.1 * d * fract(position.x),
                    position.z, 1.0);

    // Transform the position to clip coordinates
    gl_Position = ModelViewProjectionMatrix * pos;
}


