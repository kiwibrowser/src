uniform sampler2D sTexture;

varying mediump vec2  TexCoord;

void main()
{
    gl_FragColor = texture2D(sTexture, TexCoord);
}
