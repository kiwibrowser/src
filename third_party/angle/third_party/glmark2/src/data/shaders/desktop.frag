uniform sampler2D MaterialTexture0;

varying vec2 TextureCoord;

void main(void)
{
    vec4 texel = texture2D(MaterialTexture0, TextureCoord);
    gl_FragColor = texel;
}

