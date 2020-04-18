// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
struct VertexShaderInput
{
  float2 pos : POSITION;
};

struct PixelShaderInput
{
  float4 pos : SV_POSITION;
  float2 tex : TEXCOORD0;
};

PixelShaderInput flip_vertex(VertexShaderInput input)
{
  PixelShaderInput output;
  float4 pos = float4(input.pos, 1.0f, 1.0f);

  output.pos = pos;
  output.tex = (input.pos + float2(1, 1)) / float2(2, 2);

  return output;
}
