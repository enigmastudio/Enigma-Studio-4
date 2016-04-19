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
Texture2D s_texNormalG : register(t3);

SamplerState s_samTarget : register(s0);
SamplerState s_samPosG : register(s2);
SamplerState s_samNormalG : register(s3);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float  c_param;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float sinWave(float v)
{
    return sin(v) * 0.5 + 0.5;
}

float4 main(const ShaderInput input) : SV_Target0
{
    float2 texDim;
    s_texTarget.GetDimensions(texDim.x, texDim.y);

    float3 src = s_texTarget.Sample(s_samTarget, input.texCoord);
    float3 normal = normalize(s_texPosG.Sample(s_samPosG, input.texCoord));

    // stars
    const float2 rnd = rand(input.texCoord, texDim, true) * 0.1;
    float3 cam = float3(0, 0, 1);
    float c = sin(dot(normal, cam) + rnd.x);
    float s = 0;
    if (rnd.x > 0.099) s = rnd.y * 5;

    // nebula
    float3 col = float3(
        sinWave((normal.x + (src.x - 0.5)) * 11.352), 
        sinWave((normal.y + (src.y - 0.5)) * 8.437),
        sinWave((normal.z + (src.z - 0.5)) * 13.45)
        );

    float3 col2 = float3(
        sinWave((col.x + (src.x - 0.5)) * 5.32), 
        sinWave((col.y + (src.y - 0.5)) * 8.437),
        sinWave((col.z + (src.z - 0.5)) * 3.15)
        );

    float3 col3 = float3(
        sinWave((col2.x + (col.x - 0.5)) * 7.27), 
        sinWave((col2.y + (col.y - 0.5)) * 4.467),
        sinWave((col2.z + (col.z - 0.5)) * 2.96)
        );

    return float4(col3 * float3(0.05, 0.3, 0.4) * 0.4 + s, 1.0f);


    //return saturate(float4(normal, 1.0f) - float4(c, c, c, 0.0)) + float4(s, s, s, 1.0);
}