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

Texture2D s_texDiffG : register(t0);
Texture2D s_texSpecG : register(t1);
Texture2D s_texPosG : register(t2);
Texture2D s_texNormalG : register(t3);

SamplerState s_samDiffG : register(s0);
SamplerState s_samSpecG : register(s1);
SamplerState s_samPosG : register(s2);
SamplerState s_samNormalG : register(s3);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float2 c_depthRange;
};

struct ShaderInput
{
	float4 hpos:    SV_Position;
	float2 uv:      TEXCOORD0;
};

float4 main(const ShaderInput input) : SV_Target0
{   
    float4 color = float4(0, 0, 1, 1);

    if (input.uv.x < 0.5) {
        if (input.uv.y < 0.5) {
            color = s_texDiffG.Sample(s_samDiffG, input.uv * 2);
        } else {
            color = s_texSpecG.Sample(s_samSpecG, (input.uv - float2(0.0, 0.5)) * 2);
        }
    } else {
        if (input.uv.y < 0.5) {
            color = (length(s_texPosG.Sample(s_samPosG, (input.uv - float2(0.5, 0.0)) * 2)) - c_depthRange.x) / (c_depthRange.y - c_depthRange.x);
        } else {
            color = s_texNormalG.Sample(s_samNormalG, (input.uv - float2(0.5, 0.5)) * 2);
        }
    }
    
    return color;
}