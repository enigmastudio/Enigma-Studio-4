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
    float4 pos:         POSITION;
    float4 color:       COLOR;
    float2 texCoord:    TEXCOORD0;
};

struct ShaderOutput
{
    float4 hpos:        SV_Position;// position (clip-space)
    float2 texCoord:    TEXCOORD0;  // texture coordinate
    float3 vpos:        TEXCOORD1;  // position (view-space)
    float3 normal:      TEXCOORD2;  // normal (view-space)
    float4 color:       COLOR0;     // color
};


ShaderOutput main(const ShaderInput input)
{
	ShaderOutput output = (ShaderOutput)0;
	
	output.hpos     = mul(input.pos, c_mvpMtx);
	output.color    = input.color;
    output.texCoord = input.texCoord;

	return output;
}