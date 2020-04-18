#ifdef GL_ES
precision highp float;
#endif
  
uniform sampler2D uSampler;
uniform sampler2D uSampler1;
uniform float uCurrentTime;
  
varying vec2 vTextureCoord;
varying vec4 vWorld;
varying vec3 vDiffuse;
varying vec3 vAmbient;
varying vec3 vFresnel;

void main(void)
{
    vec4 caustics = texture2D(uSampler1, vec2(vWorld.x / 24.0 + uCurrentTime / 20.0, (vWorld.z - vWorld.y)/48.0 + uCurrentTime / 40.0));
    vec4 colorMap = texture2D(uSampler, vTextureCoord);
    float transparency = colorMap.a + pow(vFresnel.r, 2.0) - 0.3;
    gl_FragColor = vec4(((vAmbient + vDiffuse + caustics.rgb) * colorMap.rgb), transparency);
}
