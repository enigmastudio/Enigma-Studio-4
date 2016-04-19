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

RWTexture2D<float4> output: register(u0);

cbuffer Params : register(b0)
{
    int     octaves:    packoffset(c2.x);
    int     frequency:  packoffset(c3.x);
    float   persis:     packoffset(c4.x);
    float   amplify:    packoffset(c5.x);
    float4  col0:       packoffset(c6.x);
    float4  col1:       packoffset(c7.x);
    int     seed:       packoffset(c8.x);
};

// http://www.gamedev.net/topic/502913-fast-computed-noise/

float2 random2 (const float2 x)
{
    float2 z = fmod(fmod(x+seed, float2(5612,5612)), float2(eTWOPI,eTWOPI));
    return frac ((z*z) * float2(56812.5453,56812.5453)/(seed+1));
}

static const float A = 1.0, B = 57.0;
static const float2 AB = float2(A, B), A1 = float2(0, B);

float cnoise (const in float2 xx, int wrap)
{
    float2 fx = frac(xx), ix = xx-fx, wx = fx*fx*(3.0-2.0*fx),
    N1 = dot(ix, AB) + A1, N2 = N1 + A, N = N1,
    cmp = (ix == wrap) ? float2(1,1) : float2(0,0);

    if (any(cmp))
    {
        N1.y = A*ix.x;
        N2 = N1+A;

	    if (cmp.x)
	    {
		    N2 = B*ix.y + A1;

            if (cmp.y)
		        N2.y = 0;
            else
        		N1 = N;
	    }
    }

	fx = lerp(random2(N1), random2(N2), wx.x);
	return 1.0 - 2.0 * lerp(fx.x, fx.y, wx.y);
}

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint width, height;
    output.GetDimensions(width, height);

    float2 p = float2(threadId.xy)/float2(width, height);

    float f = 0, freq = 1<<frequency, invPersis = 1.0f/persis, scale = 1.0f/amplify;

    for( int i=0; i < octaves; i++, freq*=2.0f, scale*=invPersis)
        f += scale * cnoise(p*freq, freq-1);

	uint4 col = uint4(lerp(col0, col1, (f+1)*0.5)*255);
    output[threadId.xy] = float4(col)/255;
}