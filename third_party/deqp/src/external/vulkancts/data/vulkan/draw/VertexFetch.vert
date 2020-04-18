#version 430

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in int in_refVertexIndex;

layout(location = 0) out vec4 out_color;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	gl_Position = in_position;
	if (gl_VertexIndex == in_refVertexIndex)
		out_color = in_color;
	else
		out_color = vec4(1.0, 0.0, 0.0, 1.0);
}