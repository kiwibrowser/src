uniform sampler2D MaterialTexture0;

varying vec4 Color;
varying vec2 TextureCoord;

const vec2 TexelSize = vec2(1.0 / TextureSize.x, 1.0 / TextureSize.y); 

/*
 * See http://www.gamerendering.com/2008/10/05/bilinear-interpolation/
 * for a more thorough explanation of how this works.
 */
vec4 texture2DBilinear(sampler2D sampler, vec2 uv)
{
    // Get the needed texture samples
    vec4 tl = texture2D(sampler, uv);
    vec4 tr = texture2D(sampler, uv + vec2(TexelSize.x, 0));
    vec4 bl = texture2D(sampler, uv + vec2(0, TexelSize.y));
    vec4 br = texture2D(sampler, uv + vec2(TexelSize.x , TexelSize.y));

    // Mix the samples according to the GL specification
    vec2 f = fract(uv * TextureSize);
    vec4 tA = mix(tl, tr, f.x);
    vec4 tB = mix(bl, br, f.x);
    return mix(tA, tB, f.y);
}

void main(void)
{
    vec4 texel = texture2DBilinear(MaterialTexture0, TextureCoord);
    gl_FragColor = texel * Color;
}
