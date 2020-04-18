attribute vec3 position;

varying vec2 TextureCoord;

void main(void)
{
    gl_Position = vec4(position, 1.0);

    TextureCoord = position.xy * 0.5 + 0.5;
}
