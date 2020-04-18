
precision highp float;

uniform sampler2D GLGE_EMIT;

varying vec2 texCoord;

float blurSize=0.007;


void main(void){
	vec4 color=vec4(0.0,0.0,0.0,0.0);
	color += texture2D(GLGE_EMIT, vec2(texCoord.x - 4.0*blurSize, texCoord.y)) * 0.05;
	color += texture2D(GLGE_EMIT, vec2(texCoord.x - 3.0*blurSize, texCoord.y)) * 0.09;
	color += texture2D(GLGE_EMIT, vec2(texCoord.x - 2.0*blurSize, texCoord.y)) * 0.12;
	color += texture2D(GLGE_EMIT, vec2(texCoord.x - blurSize, texCoord.y)) * 0.15;
	color += texture2D(GLGE_EMIT, vec2(texCoord.x, texCoord.y)) * 0.16;
	color += texture2D(GLGE_EMIT, vec2(texCoord.x + blurSize, texCoord.y)) * 0.15;
	color += texture2D(GLGE_EMIT, vec2(texCoord.x + 2.0*blurSize, texCoord.y)) * 0.12;
	color += texture2D(GLGE_EMIT, vec2(texCoord.x + 3.0*blurSize, texCoord.y)) * 0.09;
	color += texture2D(GLGE_EMIT, vec2(texCoord.x + 4.0*blurSize, texCoord.y)) * 0.05;
    gl_FragColor = vec4(color.rgb,1.0);
}
	
