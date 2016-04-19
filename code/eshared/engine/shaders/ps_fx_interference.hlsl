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

Texture2D s_texTarget : register(t0);
SamplerState s_samTarget : register(s0);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float2 c_moveR;
    float2 c_moveG;
    float2 c_moveB;
    float2 c_amount;
    float  c_time;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float4 main(const ShaderInput input) : SV_Target0
{
    float2 texDim;
    s_texTarget.GetDimensions(texDim.x, texDim.y);

    const float offset = sin(c_time) * 0.01;
    const float2 rnd_r = (c_moveR + rand(input.texCoord + offset * 0.1, texDim, true) * c_amount) * c_amount;
    const float2 rnd_g = (c_moveG + rand(input.texCoord + offset * 0.2, texDim, true) * c_amount) * c_amount;
    const float2 rnd_b = (c_moveB + rand(input.texCoord + offset * 0.3, texDim, true) * c_amount) * c_amount;

    float3 color;
    
    color.r = s_texTarget.Sample(s_samTarget, input.texCoord + rnd_r).r;
    color.g = s_texTarget.Sample(s_samTarget, input.texCoord + rnd_g).g;
    color.b = s_texTarget.Sample(s_samTarget, input.texCoord + rnd_b).b;

    return float4(color, 1.0);
}