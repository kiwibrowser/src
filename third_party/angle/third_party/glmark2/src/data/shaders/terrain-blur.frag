uniform sampler2D Texture0;

varying vec2 vUv;

void main(void)
{
    vec4 result;
    vec2 TextureCoord = vUv;

    $CONVOLUTION$

    gl_FragColor = vec4(result.xyz, 1.0);
}

