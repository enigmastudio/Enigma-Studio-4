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


Texture2D<float4>   input0: register(t0);
Texture2D<float4>   input1: register(t1);
Texture2D<float4>   mask:   register(t2);
RWTexture2D<float4> output: register(u0);

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint2 xy = threadId.xy;

    float4 col0 = input0[xy];
    float4 col1 = input1[xy];
    float4 cov = mask[xy];

    col0 *= cov;
    col1 *= (float4(1.0, 1.0f, 1.0f, 1.0f)-mask[xy]);
    output[xy] = col0+col1;
}