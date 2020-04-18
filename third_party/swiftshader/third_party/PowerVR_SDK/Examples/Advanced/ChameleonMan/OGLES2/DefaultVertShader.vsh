attribute highp   vec3 inVertex;
attribute mediump vec2 inTexCoord;

uniform highp   mat4 MVPMatrix;
uniform float	fUOffset;

varying mediump vec2  TexCoord;

void main()
{
	gl_Position = MVPMatrix * vec4(inVertex, 1.0);

	// Pass through texcoords
	TexCoord = inTexCoord;
	TexCoord.x += fUOffset;
}
 