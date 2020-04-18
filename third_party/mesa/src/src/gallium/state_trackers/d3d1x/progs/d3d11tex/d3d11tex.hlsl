/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

Texture2D tex0;
Texture2D tex1;
sampler samp0;
sampler samp1;

struct IA2VS
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
};

struct VS2PS
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float4 factors : FACTORS;
};

VS2PS vs(IA2VS input)
{
	VS2PS result;
	result.position = input.position;
	result.texcoord = input.texcoord * 8;
	result.factors.xy = input.texcoord;
	result.factors.zw = 1 - input.texcoord;
	return result;
}

float4 ps(VS2PS input) : SV_TARGET
{
	float4 a0 = tex0.Sample(samp0, input.texcoord);
	float4 a1 = tex0.Sample(samp1, input.texcoord);
	float4 a = a0 * input.factors.z + a1 * input.factors.x;

	float4 b0 = tex1.Sample(samp0, input.texcoord);
	float4 b1 = tex1.Sample(samp1, input.texcoord);
	float4 b = b0 * input.factors.z + b1 * input.factors.x;

	return a * input.factors.w + b * input.factors.y;
}
