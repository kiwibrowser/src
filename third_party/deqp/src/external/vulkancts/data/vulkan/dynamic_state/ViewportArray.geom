#version 450
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in gl_PerVertex { vec4 gl_Position; } gl_in[];
out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) in vec4 in_color[];
layout(location = 0) out vec4 out_color;

void main() {
	for (int i=0; i<gl_in.length(); ++i) {
		gl_Position = gl_in[i].gl_Position;
		gl_ViewportIndex = int(round(gl_in[i].gl_Position.z * 3.0));
		out_color = in_color[i];
		EmitVertex();
	}
	EndPrimitive();
}
