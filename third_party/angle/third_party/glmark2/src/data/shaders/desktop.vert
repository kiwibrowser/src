attribute vec2 position;
attribute vec2 texcoord;

varying vec2 TextureCoord;

void main(void)
{
    gl_Position = vec4(position, 0.0, 1.0);

    TextureCoord = texcoord;
}
