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
    uint    mode:   packoffset(c0.x);
    float4  col:    packoffset(c1.x);
};

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    float4 res = input[threadId.xy];

    if (mode == 0) // add
        res += col;
    else if (mode == 1) // subtract
        res -= col;
    else if (mode == 2) // multiply
        res *= col;
    else if (mode == 4) // invert (ignores color, preserves alpha)
        res.rgb = 1.0f-res.rgb;
    else if (mode == 3) // grayscale (ignores color)
    {
        float c = grayScale(res.rgb);
        res.rgb = float3(c, c, c);
    }

    output[threadId.xy] = res;
}