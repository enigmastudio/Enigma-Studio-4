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
    float4          c_color;
	float4			c_position;
	float2			c_size;
	float			c_power;
	int				c_occlusionTest;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float testOcclusion(float2 pos, float test_depth, float factor)
{
	float4 depth = s_texPosG.Sample(s_samPosG, pos);

	if (depth.z < 0.0001)
		return 0;

	if (depth.z > test_depth)
		factor += 1.0f;

	return factor;
}

float4 main(const ShaderInput input) : SV_Target0
{
	float2 texDim;
    s_texTarget.GetDimensions(texDim.x, texDim.y);
    texDim.x = 1.0 / texDim.x;
    texDim.y = 1.0 / texDim.y;
	texDim *= 4.0f;

	float occlusion = 0.0f;
	if (c_occlusionTest)
	{
		occlusion = testOcclusion(c_position.xy, c_position.z, occlusion);
		occlusion = testOcclusion(c_position.xy + float2(texDim.x, 0), c_position.z, occlusion);
		occlusion = testOcclusion(c_position.xy + float2(texDim.x, texDim.y), c_position.z, occlusion);
		occlusion = testOcclusion(c_position.xy + float2(0, texDim.y), c_position.z, occlusion);
		occlusion = testOcclusion(c_position.xy + float2(-texDim.x, texDim.y), c_position.z, occlusion);
		occlusion = testOcclusion(c_position.xy + float2(-texDim.x, 0), c_position.z, occlusion);
		occlusion = testOcclusion(c_position.xy + float2(-texDim.x, -texDim.y), c_position.z, occlusion);
		occlusion = testOcclusion(c_position.xy + float2(0, -texDim.y), c_position.z, occlusion);
		occlusion = testOcclusion(c_position.xy + float2(texDim.x, -texDim.y), c_position.z, occlusion);
		occlusion /= 9.0f;
	}
	else
	{
		occlusion = 1;
	}

	float4 col = s_texTarget.Sample(s_samTarget, input.texCoord);
    
	if (occlusion > 0.0)
	{
		float2 dist_vec = input.texCoord - c_position.xy;
		dist_vec *= (1.0 / c_size);
		float dist = length(dist_vec);
		float inv_dist = saturate(1.0 - dist);
		col.rgb += (pow(inv_dist, c_power) * c_color * occlusion).rgb;
	}

    return col;
}