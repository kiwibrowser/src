#version 300 es

uniform  sampler2D      sTexture;

in mediump vec2 texCoords;
layout(location = 0) out lowp vec4 oFragColour;

void main()
{
    highp vec3 vCol = texture(sTexture, texCoords).rgb;
    oFragColour = vec4(vCol, 1.0);
}
