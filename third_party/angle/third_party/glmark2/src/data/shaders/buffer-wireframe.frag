varying vec4 dist;

const vec4 LINE_COLOR = vec4(1.0);
const vec4 TRIANGLE_COLOR = vec4(0.0, 0.5, 0.8, 0.8);

void main(void)
{
    // Get the minimum distance of this fragment from a triangle edge.
    // We need to multiply with dist.w to undo the workaround we had
    // to perform to get linear interpolation (instead of perspective correct).
    float d = min(dist.x * dist.w, min(dist.y * dist.w, dist.z * dist.w));

    // Get the intensity of the wireframe line
    float I = exp2(-2.0 * d * d);

    gl_FragColor = mix(TRIANGLE_COLOR, LINE_COLOR, I);
}
