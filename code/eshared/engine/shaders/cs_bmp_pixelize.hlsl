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


Texture2D<float4>   input:  register(t0);
RWTexture2D<float4> output: register(u0);

cbuffer Params : register(b0)
{
    int2  amount:     packoffset(c0.x);
};

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint2 xy = int2(threadId.xy);
    uint w, h;
    output.GetDimensions(w, h);
    uint2 wh = uint2(w, h);

	uint2 xy_small = xy / amount;
	uint2 xy_upscaled = xy_small * amount;

	float4 col;
	for(int x=0;x<amount.x;x++)
	{
		for(int y=0;y<amount.y;y++)
		{
			col += input[xy_upscaled + int2(x,y)];
		}
	}

	col /= amount.x * amount.y;
    output[xy] = col;
}