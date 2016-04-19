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

Texture2D s_texDiffuseG : register(t0);
Texture2D s_texNormalG : register(t1);
Texture2D s_texSpecularG : register(t2);
Texture2D s_texPositionG : register(t3);
Texture2D s_texDefShadowMap : register(t4);

SamplerState s_samDiffuseG : register(s0);
SamplerState s_samNormalG : register(s1);
SamplerState s_samSpecularG : register(s2);
SamplerState s_samPositionG : register(s3);
SamplerState s_samDefShadowMap : register(s4);

struct ShaderInput
{
    float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float4 main(const ShaderInput input) : SV_Target0
{
    // do shadowing
	const float occlusion = s_texDefShadowMap.Sample(s_samDefShadowMap, input.texCoord).r;
    if (occlusion == 1.0f)
        return float4(0.0f, 0.0f, 0.0f, 1.0f);

    // check if geometry was rendered at
	// pixel. if not return clear color.
	const float3 pos = s_texPositionG.Sample(s_samPositionG, input.texCoord).xyz;
	if (pos.z == 0.0f)
		return c_camClearColor;

	// evaluate lighting model
	const float3 normal = s_texNormalG.Sample(s_samNormalG, input.texCoord).xyz;
	const float3 diffCol = s_texDiffuseG.Sample(s_samDiffuseG, input.texCoord).xyz;
	const float4 specCol = s_texSpecularG.Sample(s_samSpecularG, input.texCoord);

	const float3 col = (1.0f-occlusion)*phong(normal, c_lightViewPos, pos, c_lightInvRange, c_lightDiffuseCol,
                                              c_lightSpecCol, diffCol, specCol.xyz, specCol.w);
    return float4(col, 0.0);
}