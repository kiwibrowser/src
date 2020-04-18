uniform sampler2D tDiffuse;

varying vec2 vUv;

void main()
{
    vec4 texel = texture2D( tDiffuse, vUv );

    vec3 luma = vec3( 0.299, 0.587, 0.114 );

    float v = dot( texel.xyz, luma );

    gl_FragColor = vec4( v, v, v, texel.w );
}

