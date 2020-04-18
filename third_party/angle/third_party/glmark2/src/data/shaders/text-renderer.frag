uniform sampler2D Texture0;

varying vec2 TextureCoord;

void main(void)
{
    vec4 texel = texture2D(Texture0, TextureCoord);
    gl_FragColor = texel;
}

