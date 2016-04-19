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
    float4 pos:         POSITION0;
    float2 texCoord:    TEXCOORD0;
    float4 color:       COLOR0;
};

struct ShaderOutput
{
	float4 pos:         SV_Position;
	float2 texCoord:    TEXCOORD0;
	float4 color:       COLOR0;
};

ShaderOutput main(const ShaderInput input)
{
	ShaderOutput output = (ShaderOutput)0;
	
	output.pos = mul(input.pos, c_mvpMtx);
	output.texCoord = input.texCoord;
	output.color = input.color;

	return output;
}