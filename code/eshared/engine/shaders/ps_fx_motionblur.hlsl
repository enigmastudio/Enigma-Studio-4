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
Texture2D s_texPosG : register(t2);

SamplerState s_samTarget : register(s0);
SamplerState s_samPosG : register(s2);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float4x4        c_invViewMatrix;
    float4x4        c_lastViewMatrix;
    float4x4        c_projMatrix;
    float           c_strength;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float4 main(const ShaderInput input) : SV_Target0
{
    const float4 pos = float4(s_texPosG.Sample(s_samPosG, input.texCoord).xyz, 1.0);
    const float4 worldPos = mul(pos, c_invViewMatrix);
    const float4 oldPos = mul(worldPos, c_lastViewMatrix);

    const float4 projPos = mul(pos, c_projMatrix);
    const float4 projOldPos = mul(oldPos, c_projMatrix);
    const float2 velocity = ((projPos.xy / projPos.z) - (projOldPos.xy / projOldPos.z)) * float2(1, -1) * c_strength * 0.01;

    float depth = length(pos);
    depth *= 1.01;

    //return float4(velocity.xy, 0.0, 1.0);

	float4 src = s_texTarget.Sample(s_samTarget, input.texCoord);
    float4 color = src;
    float div = 1.0;

    for (int i=0; i<16; i++)
    {
        const float2 lookupPos = input.texCoord + velocity * i;

        //if (length(s_texPosG.Sample(s_samPosG, lookupPos)) <= depth)
        {
            float f = 1.0 - (1.0 / 16.0) * i;
            color += s_texTarget.Sample(s_samTarget, lookupPos) * f;
            div += f;
        }
    }

    for (int i=0; i<16; i++)
    {
        const float2 lookupPos = input.texCoord - velocity * i;

        //if (length(s_texPosG.Sample(s_samPosG, lookupPos)) <= depth)
        {
            float f = 1.0 - (1.0 / 16.0) * i;
            color += s_texTarget.Sample(s_samTarget, lookupPos) * f;
            div += f;
        }
    }

    return float4(color.xyz / div, src.a);
}