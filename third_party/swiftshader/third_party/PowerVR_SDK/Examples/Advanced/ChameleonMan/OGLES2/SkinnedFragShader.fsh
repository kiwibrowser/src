uniform sampler2D sTexture;
uniform sampler2D sNormalMap;
uniform bool bUseDot3;

varying mediump vec2 TexCoord;
varying mediump vec3 Light;

void main()
{
	if(bUseDot3)
	{
		/*
			Note:
			In the normal map red = y, green = x, blue = z which is why when we get the normal
			from the texture we use the swizzle .grb so the colours are mapped to the correct
			co-ordinate variable.
		*/

		mediump vec3 fNormal = texture2D(sNormalMap, TexCoord).grb;
		mediump float fNDotL = dot((fNormal - 0.5) * 2.0, Light);
		
		gl_FragColor = texture2D(sTexture, TexCoord) * fNDotL;
    }
    else
		gl_FragColor = texture2D(sTexture, TexCoord) * Light.x;
}
