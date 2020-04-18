attribute vec2 position;

uniform mat4 LightMatrix;
uniform mat4 ModelViewProjectionMatrix;

varying vec4 ShadowCoord;
varying vec4 Color;

void main()
{
    Color = MaterialDiffuse;

    vec4 pos4 = vec4(position, 0.0, 1.0);
    ShadowCoord = LightMatrix * pos4;
    gl_Position = ModelViewProjectionMatrix * pos4;
}
