attribute vec3 position;
attribute vec4 vtxcolor;
attribute vec2 texcoord;
attribute vec3 normal;

uniform mat4 ModelViewProjectionMatrix;

varying vec4 Color;
varying vec2 TextureCoord;

void main(void)
{
    Color = vtxcolor;
    TextureCoord = texcoord;

    // Transform the position to clip coordinates
    gl_Position = ModelViewProjectionMatrix * vec4(position, 1.0);
}

