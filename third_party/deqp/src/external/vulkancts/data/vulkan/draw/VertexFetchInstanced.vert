#version 430

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in int in_refVertexIndex;

layout(location = 0) out vec4 out_color;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	vec2 perVertex = vec2(in_position.x, in_position.y);
	vec2 perInstance[6]	= vec2[6](vec2(0.0, 0.0), vec2(0.3, 0.0), vec2(0.0, -0.3),vec2(0.3, -0.3), vec2(0.7, -0.7), vec2(-0.75, 0.8));

	gl_Position = vec4(perVertex + perInstance[gl_InstanceIndex], 0.0, 1.0);

	if (gl_VertexIndex == in_refVertexIndex)
		out_color = in_color;
	else
		out_color = vec4(1.0, 0.0, 0.0, 1.0);
}
