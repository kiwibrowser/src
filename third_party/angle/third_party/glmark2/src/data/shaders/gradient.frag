uniform vec3 color1;
uniform vec3 color2;

varying vec2 uv;

void main()
{
    vec3 color = mix(color1, color2, uv.x * uv.y);
    gl_FragColor = vec4(color, 1.0);
}
