attribute highp   vec3  inVertex;
attribute mediump vec3  inNormal;
attribute mediump vec2  inTexCoord;

uniform highp   mat4  MVPMatrix;
uniform mediump vec3  LightDirection;
uniform mediump	float  DisplacementFactor;

varying lowp    float  LightIntensity;
varying mediump vec2   TexCoord;

uniform sampler2D  sDisMap;

void main()
{
	/* 
		Calculate the displacemnt value by taking the colour value from our texture
		and scale it by out displacement factor.
	*/
	mediump float disp = texture2D(sDisMap, inTexCoord).r * DisplacementFactor;

	/* 
		Transform position by the model-view-projection matrix but first
		move the untransformed position along the normal by our displacement
		value.
	*/
	gl_Position = MVPMatrix * vec4(inVertex + (inNormal * disp), 1.0);

	// Pass through texcoords
	TexCoord = inTexCoord;
	
	// Simple diffuse lighting in model space
	LightIntensity = dot(inNormal, -LightDirection);
}