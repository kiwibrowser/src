
precision highp float;

uniform sampler2D GLGE_PASS0;
uniform sampler2D GLGE_RENDER;

varying vec2 texCoord;

float blurSize=0.007;



void main(void){
	vec4 color=vec4(0.0,0.0,0.0,0.0);
	color += texture2D(GLGE_PASS0, vec2(texCoord.x, texCoord.y - 4.0*blurSize)) * 0.05;
	color += texture2D(GLGE_PASS0, vec2(texCoord.x, texCoord.y - 3.0*blurSize)) * 0.09;
	color += texture2D(GLGE_PASS0, vec2(texCoord.x, texCoord.y - 2.0*blurSize)) * 0.12;
	color += texture2D(GLGE_PASS0, vec2(texCoord.x, texCoord.y - blurSize)) * 0.15;
	color += texture2D(GLGE_PASS0, vec2(texCoord.x, texCoord.y)) * 0.16;
	color += texture2D(GLGE_PASS0, vec2(texCoord.x, texCoord.y + blurSize)) * 0.15;
	color += texture2D(GLGE_PASS0, vec2(texCoord.x, texCoord.y + 2.0*blurSize)) * 0.12;
	color += texture2D(GLGE_PASS0, vec2(texCoord.x, texCoord.y + 3.0*blurSize)) * 0.09;
	color += texture2D(GLGE_PASS0, vec2(texCoord.x, texCoord.y + 4.0*blurSize)) * 0.05;
	gl_FragColor = vec4(color.rgb*3.0+texture2D(GLGE_RENDER,texCoord).rgb,1.0);
}
	
