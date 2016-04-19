
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
	float4x4    c_invViewMatrix;
	float4x4    c_transViewMatrix;
	float		c_scale;
	float		c_time;
	float		c_amp;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float mod(float x, float y)
{
  return x - y * floor(x/y);
}

float4 main(const ShaderInput input) : SV_Target0
{
	float rand[] = { 1.3456, 2.137, 3.753, 2.4566 };

    float2 texDim;
    s_texPosG.GetDimensions(texDim.x, texDim.y);

    const float4 pos = float4(s_texPosG.Sample(s_samPosG, input.texCoord).xyz, 1.0);
	const float4 worldPos = mul(pos, c_invViewMatrix);
	const float4 nrm = float4(s_texNormalG.Sample(s_samNormalG, input.texCoord).xyz, 0.0);
	const float nrm_y = mul(nrm, c_transViewMatrix).y;

	float caustic_color = 0.0f;
	int i = 1;

	float ipow = 2.0;
	for(i=1; i<4;i++)
	{
		const float x = (worldPos.x + c_time * rand[i]) * c_scale;
		const float y = (worldPos.z + c_time * rand[i]) * c_scale;
		caustic_color += (sin(x * ipow) + sin(y * ipow)) * c_amp;
		ipow *= 2;
	}

	caustic_color = saturate(1.0f - abs(caustic_color - 0.5));
	caustic_color *= nrm_y * 0.5;
	caustic_color += 0.5;

    return float4(caustic_color, caustic_color, caustic_color, 1.0);
}