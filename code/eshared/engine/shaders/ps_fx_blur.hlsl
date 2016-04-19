/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *   This file is part of
 *       ______        _                             __ __
 *      / ____/____   (_)____ _ ____ ___   ____ _   / // /
 *     / __/  / __ \ / // __ `// __ `__ \ / __ `/  / // /_
 *    / /___ / / / // // /_/ // / / / / // /_/ /  /__  __/
 *   /_____//_/ /_//_/ \__, //_/ /_/ /_/ \__,_/     /_/.   
 *                    /____/                              
 *
 *   Copyright © 2003-2012 Brain Control, all rights reserved.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "globals.hlsli"

Texture2D    g_tex: register(t0);
SamplerState g_ss:  register(s0);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float2 c_dist;
};

struct ShaderInput
{
	float4 hpos:    SV_Position;
	float2 uv:      TEXCOORD0;
};

// gaussian blur approximation using linear
// sampling (requires less texture fetches)
static const float WEIGHTS[3] = {0.2270270270f, 0.3162162162f, 0.0702702703f};

float4 main(const ShaderInput input) : SV_Target0
{
    float2 texDim;
    g_tex.GetDimensions(texDim.x, texDim.y);

    const float2 adj = c_dist/texDim;
    const float2 offset[3] = {float2(0.0f, 0.0f), 1.3846153846f*adj, 3.2307692308f*adj};

	float4 col = g_tex.Sample(g_ss, input.uv);
    float4 res = col*WEIGHTS[0];
    res += (g_tex.Sample(g_ss, input.uv+offset[1])+g_tex.Sample(g_ss, input.uv-offset[1]))*WEIGHTS[1];
    res += (g_tex.Sample(g_ss, input.uv+offset[2])+g_tex.Sample(g_ss, input.uv-offset[2]))*WEIGHTS[2];

    return float4(res.rgb, col.a);
}