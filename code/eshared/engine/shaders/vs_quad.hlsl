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

struct ShaderInput
{
    float3 pos:         POSITION;
    float2 texCoord:    TEXCOORD0;
};

struct ShaderOutput 
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

ShaderOutput main(const ShaderInput input)
{
	ShaderOutput output;
	
	output.hpos     = mul(float4(input.pos, 1.0f), c_mvpMtx);
	output.texCoord = input.texCoord;

	return output;
}