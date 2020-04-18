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

cbuffer cb
{
	float4x4 proj;
	float4x4 modelview;
	float4 light;
	float4 diffuse;
	float4 specular;
	float specular_power;
};

struct IA2VS
{
	float4 position : POSITION;
	float3 normal : NORMAL;
};

struct VS2PS
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 eye : EYE;
	float3 light : LIGHT;
};

VS2PS vs(IA2VS input)
{
	VS2PS result;

	float3 view = mul((float3x4)modelview, input.position);
	result.position = mul((float4x4)proj, float4(view, 1));
	result.light = light - view;
	result.eye = -view;
	result.normal = mul((float3x3)modelview, input.normal);

	return result;
}

float4 ps(VS2PS input) : SV_TARGET
{
	float3 nlight = normalize(input.light);
	float3 nnormal = normalize(input.normal);

	float diffuse_c = saturate(dot(nnormal, nlight));
	float specular_c = pow(saturate(dot(nnormal, normalize(normalize(input.eye) + nlight))), specular_power);

	return diffuse * diffuse_c + specular * specular_c;
}


