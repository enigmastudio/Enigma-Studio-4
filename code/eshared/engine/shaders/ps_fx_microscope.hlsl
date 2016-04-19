
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
    float4  c_foregroundColor1;
	float4  c_foregroundColor2;
    float4  c_backgroundColor;
    float   c_depth;
    float   c_noiseAmount;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float sampleNrmZNeg(float2 uv, float2 pixdim)
{
	float z = 0;

	for(int x=-1;x<2;x++)
	{
		for(int y=-1;y<2;y++)
		{
			z += s_texNormalG.Sample(s_samNormalG, uv + float2(x, y) * pixdim).z;
		}
	}
	
	return -(z/9);
}

float4 main(const ShaderInput input) : SV_Target0
{
    float2 texDim;
    s_texPosG.GetDimensions(texDim.x, texDim.y);

    const float delta_x = 1.0f / texDim.x;
    const float delta_y = 1.0f / texDim.y;
    const float3 pos = s_texPosG.Sample(s_samPosG, input.texCoord).xyz;
    const float2 rnd = rand(input.texCoord, texDim, true) * c_noiseAmount;
    float depth = saturate(1.0 - (length(pos) / c_depth));

	float x = input.texCoord.x;
	float y = input.texCoord.y;

    float n = sampleNrmZNeg(input.texCoord, float2(delta_x, delta_y));
    float nl = sampleNrmZNeg(float2(x - delta_x,	y), float2(delta_x, delta_y));
    float nr = sampleNrmZNeg(float2(x + delta_x,	y), float2(delta_x, delta_y));
    float nt = sampleNrmZNeg(float2(x,				y - delta_y), float2(delta_x, delta_y));
    float nb = sampleNrmZNeg(float2(x,				y + delta_y), float2(delta_x, delta_y));

    nl += rnd.x;
    nt += rnd.x;

    float grey = depth * (1.0f - (n - (nl - nr) - (nt - nb)));
    grey *= grey;

	float4 bg = float4(c_backgroundColor.xyz * (1.0-grey), length(pos));
	float4 fg1 = float4(c_foregroundColor1.xyz * grey * depth, 0.0f);
	float4 fg2 = float4(c_foregroundColor2.xyz * grey * (1.0 - depth), 0.0f);
	
    return bg + fg1 + fg2;
}