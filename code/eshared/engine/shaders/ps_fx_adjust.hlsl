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
    float3  c_adjCol; // = color*brightess
    float3  c_subCol;
    float3  c_addCol;
    float   pad;
    float   c_contrast;
};

struct ShaderInput
{
	float4 hpos:    SV_Position;
	float2 uv:      TEXCOORD0;
};

float4 main(const ShaderInput input) : SV_Target0
{
    const float4 col = g_tex.Sample(g_ss, input.uv);
    float3 res = col.rgb*c_adjCol;
    res = (res-0.5f)*c_contrast+0.5f;
    res -= c_subCol;
    res += c_addCol;
    return float4(res, 1.0);
}