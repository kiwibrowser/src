varying vec4 dummy;

void main(void)
{
    float d = fract(gl_FragCoord.x * gl_FragCoord.y * 0.0001);

$MAIN$

    gl_FragColor = vec4(d, d, d, 1.0);
}

