#version 300 es

uniform  sampler2D     sTexture;
uniform  mediump sampler3D		sColourLUT;

in mediump vec2 texCoords;
layout(location = 0) out lowp vec4 oFragColour;

void main()
{
    highp vec3 vCol = texture(sTexture, texCoords).rgb;
	lowp vec3 vAlteredCol = texture(sColourLUT, vCol.rgb).rgb;
    oFragColour = vec4(vAlteredCol, 1.0);
}
