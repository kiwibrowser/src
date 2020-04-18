uniform sampler2D sTexture;

varying lowp    vec3  DiffuseLight;
varying lowp    vec3  SpecularLight;
varying mediump vec2  TexCoord;

void main()
{
	lowp vec3 texColor  = texture2D(sTexture, TexCoord).rgb;
	lowp vec3 color = (texColor * DiffuseLight) + SpecularLight;
	gl_FragColor = vec4(color, 1.0);
}

