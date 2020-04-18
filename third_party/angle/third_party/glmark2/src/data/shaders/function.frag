varying vec4 dummy;

float process(float d)
{
$PROCESS$
    return d;
}

void main(void)
{
    float d = fract(gl_FragCoord.x * gl_FragCoord.y * 0.0001);

$MAIN$

    gl_FragColor = vec4(d, d, d, 1.0);
}
