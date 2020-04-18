#ifdef GL_ES
#define MEDIUMP mediump
#else
#define MEDIUMP
#endif

uniform vec2 uvOffset;
uniform MEDIUMP vec2 uvScale;

attribute vec3 position;

varying vec2 vUv;

void main()
{
    vUv = uvScale * (position.xy * 0.5 + 0.5) + uvOffset;
    gl_Position = vec4( position, 1.0);
}
