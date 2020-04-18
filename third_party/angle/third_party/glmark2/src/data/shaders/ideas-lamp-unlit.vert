uniform mat4 projection;
uniform mat4 modelview;
attribute vec3 vertex;
varying vec4 color;

void main()
{
    vec4 curVertex = vec4(vertex.x, vertex.y, vertex.z, 1.0);
    gl_Position = projection * modelview * curVertex;
    color = vec4(1.0, 1.0, 1.0, 1.0);
}
