uniform sampler2D tex;

void main()
{
    vec2 curPos = gl_FragCoord.xy / 32.0;
    vec4 color = texture2D(tex, curPos);
    if (color.w < 0.5)
        discard;
    gl_FragColor = color;
}
