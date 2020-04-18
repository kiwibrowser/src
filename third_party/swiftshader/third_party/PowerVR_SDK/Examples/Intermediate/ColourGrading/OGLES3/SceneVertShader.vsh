attribute highp vec4  inVertex;
attribute highp vec3  inNormal;
attribute highp vec2  inTexCoord;

uniform highp mat4   MVPMatrix;
uniform highp vec3   LightDirection;
uniform highp float  MaterialBias;
uniform highp float  MaterialScale;

varying lowp vec3  DiffuseLight;
varying lowp vec3  SpecularLight;
varying mediump vec2  TexCoord;

void main()
{
	gl_Position = MVPMatrix * inVertex;
	
	DiffuseLight = vec3(max(dot(inNormal, LightDirection), 0.0));
	SpecularLight = vec3(max((DiffuseLight.x - MaterialBias) * MaterialScale, 0.0));
	
	TexCoord = inTexCoord;
}
