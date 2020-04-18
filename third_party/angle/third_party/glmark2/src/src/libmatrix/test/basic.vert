attribute vec3 position;

uniform mat4 modelview;
uniform mat4 projection;

varying vec4 color;

void
main(void)
{
    vec4 curVertex = vec4(position, 1.0);
    gl_Position = projection * modelview * curVertex;
    color = ConstantColor;
}
