uniform mat4 projection;
uniform mat4 modelview;
attribute vec3 vertex;

void main()
{
    vec4 curVertex = vec4(vertex.x, vertex.y, vertex.z, 1.0);
    gl_Position = projection * modelview * curVertex;
}
