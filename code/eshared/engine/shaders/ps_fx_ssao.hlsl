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

Texture2D s_texPosG : register(t2);
Texture2D s_texNormalG : register(t3);

SamplerState s_samPosG : register(s2);
SamplerState s_samNormalG : register(s3);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float           c_scale;
    float           c_intensity;
    float           c_bias;
    float           c_radius;
    float           c_noiseAmount;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

// Calculates ambient occlusion based on angle and distance.
float ambientOcclusion(const float2 texCoord, const float3 pos, const float3 normal)
{
    const float3 diff = s_texPosG.Sample(s_samPosG, texCoord).xyz-pos;
    const float diffLen = length(diff);
    const float dist = diffLen*c_scale;

    return max(0.0f, dot(normal, diff/diffLen)-c_bias)/(1.0f+dist);
}

float4 main(const ShaderInput input) : SV_Target0
{
    const float2 vec[4] =
    {
        float2(1.0f, 0.0f), float2(-1.0f,  0.0f),
        float2(0.0f, 1.0f), float2( 0.0f, -1.0f)
    };

    float2 texDim;
    s_texPosG.GetDimensions(texDim.x, texDim.y);

    const float3 pos = s_texPosG.Sample(s_samPosG, input.texCoord).xyz;
    const float3 normal = s_texNormalG.Sample(s_samNormalG, input.texCoord).xyz;
    const float2 rnd = rand(input.texCoord, texDim, true) * c_noiseAmount;
    const float  rad = c_radius/pos.z;

    float ao = 0.0f;

    for (int i=0; i<4; i++)
	{
        // First coordiante is reflected and
        // the second rotated by 45 degrees.
        const float2 coord0 = reflect(vec[i], rnd)*rad;
        const float2 coord1 = float2(coord0.x-coord0.y, coord0.x+coord0.y)*0.707f;

        // Evaluate ambient occlusion for pixels.
        ao += ambientOcclusion(input.texCoord+0.25*coord0, pos, normal);
        ao += ambientOcclusion(input.texCoord+0.50*coord1, pos, normal);
        ao += ambientOcclusion(input.texCoord+0.75*coord0, pos, normal);
        ao += ambientOcclusion(input.texCoord+1.00*coord1, pos, normal);
    }

    ao *= c_intensity;
    return 1.0f-float4(ao, ao, ao, 0.0f);
}