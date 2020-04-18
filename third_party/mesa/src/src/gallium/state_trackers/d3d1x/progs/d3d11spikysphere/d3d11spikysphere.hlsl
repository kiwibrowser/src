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

#define INPUT_PATCH_SIZE 3
#define OUTPUT_PATCH_SIZE 3

static const float PI = 3.141592653589793238462643f;

cbuffer cb_frame
{
	float4x4 model;
	float4x4 view_proj;
	float disp_scale;
	float disp_freq;
	float tess_factor;
};

struct IA2VS
{
	float3 position : POSITION;
};

struct VS2HS
{
	float3 position : POSITION;
};

VS2HS vs(IA2VS input)
{
	VS2HS result;
	result.position = input.position;
	return result;
}

struct HS2DS_PATCH
{
	float tessouter[3] : SV_TessFactor;
	float tessinner[1] : SV_InsideTessFactor;
};

struct HS2DS
{
	float3 position : POSITION;
};

HS2DS_PATCH hs_patch(InputPatch<VS2HS, INPUT_PATCH_SIZE> ip)
{    
	HS2DS_PATCH result;

	result.tessouter[0] = result.tessouter[1] = result.tessouter[2]
		= result.tessinner[0] = tess_factor;
	return result;
}

[domain("tri")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(OUTPUT_PATCH_SIZE)]
[patchconstantfunc("hs_patch")]
HS2DS hs(InputPatch<VS2HS, INPUT_PATCH_SIZE> p, uint i : SV_OutputControlPointID)
{
	HS2DS result;
	result.position = p[i].position;
	return result;
}

struct DS2PS
{
	float4 position : SV_POSITION;
	float3 objpos : OBJPOS;
	// float3 worldpos : WORLDPOS;
	float3 objnormal : OBJNORMAL;
	float3 worldnormal : WORLDNORMAL;
};

float3 dnormf_dt(float3 f, float3 dfdt)
{
	float ff = dot(f, f);
	return (ff * dfdt - dot(f, dfdt) * f) / (ff * sqrt(ff));
}

float3 map(float3 p, float3 q, float3 r, float3 k)
{
	return normalize(p * k.x + q * k.y + r * k.z);
}

float3 dmap_du(float3 p, float3 q, float3 r, float3 k)
{
	return dnormf_dt(p * k.x + q * k.y + r * k.z, p);
}

float dispf(float v)
{
	return cos(v * disp_freq);
}

float ddispf(float v)
{
	return -sin(v * disp_freq) * disp_freq;
}

float disp(float3 k)
{
	return dispf(k.x) * dispf(k.y) * dispf(k.z);
}

float ddisp_du(float3 k)
{
	return ddispf(k.x) * dispf(k.y) * dispf(k.z);
}

float3 ddisp(float3 k)
{
	float3 f = float3(dispf(k.x), dispf(k.y), dispf(k.z));
	return float3(ddispf(k.x) * f.y * f.z, ddispf(k.y) * f.z * f.x, ddispf(k.z) * f.x * f.y);
}

[domain("tri")]
DS2PS ds(HS2DS_PATCH input, 
                    float3 k : SV_DomainLocation,
                    const OutputPatch<HS2DS, OUTPUT_PATCH_SIZE> patch)
{
	DS2PS result;

	float3 s = map(patch[0].position, patch[1].position, patch[2].position, k);
	float3 d = 1.0 + disp(s) * disp_scale;
	result.objpos = s * d;
	result.objpos /= (1.0 + disp_scale);
	float3 worldpos = mul(model, float4(result.objpos, 1.0f));
	result.position = mul(view_proj, float4(worldpos, 1.0f));
	
	float3 dd = ddisp(s) * disp_scale;

	/*
	float3 ds_du = dmap_du(patch[0].position, patch[1].position, patch[2].position, k);
	float3 ds_dv = dmap_du(patch[1].position, patch[2].position, patch[0].position, k.yzx);
	float3 ds_dw = dmap_du(patch[2].position, patch[0].position, patch[1].position, k.zxy);

	float3 ds_dU = ds_du - ds_dw;
	float3 ds_dV = ds_dv - ds_dw;

	float3 dc_dU = s * dot(dd, ds_dU) + ds_dU * d;
	float3 dc_dV = s * dot(dd, ds_dV) + ds_dV * d;
	*/

	// this should be faster
	float3 _u = normalize((abs(s.x) > abs(s.y)) ? float3(-s.z, 0, s.x) : float3(0, -s.z, s.y));	
	float3 _v = normalize(cross(s, _u));
	float3 dc_dU = s * dot(dd, _u) + _u * d;
	float3 dc_dV = s * dot(dd, _v) + _v * d;
		
	result.objnormal = normalize(cross(dc_dU, dc_dV));
	result.worldnormal = mul(model, result.objnormal);
	return result;
}

float4 ps(DS2PS input) : SV_TARGET
{
	float3 pseudoambient = float3(0.4, 0.4, 0.6);
	float3 diffuse = float3(0.6, 0.6, 0.4);
	float3 light = normalize(float3(0, 1, -1));

	float4 r;
//	r.xyz = normalize(input.objpos + 2 * input.objnormal);
	r.xyz = pseudoambient * saturate(dot(normalize(input.objnormal), normalize(input.objpos)));
	r.xyz += saturate(dot(light, normalize(input.worldnormal))) * diffuse;

	r.w = 1;
	return r;
}
