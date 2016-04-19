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
    uint2   count:      packoffset(c2.x);
    float2  shift:      packoffset(c3.x);
    float4  plasmaCol:  packoffset(c4.x);
    float4  backgCol:   packoffset(c5.x);
};

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint x = threadId.x;
    uint y = threadId.y;
    uint2 xy = int2(x, y);
    uint w, h;
    output.GetDimensions(w, h);

    float sv = sin(x*(eTWOPI/w*count.x)+shift.x*eTWOPI); // in range [-1,1]
    float cv = cos(y*(eTWOPI/h*count.y)+shift.y*eTWOPI);
    float t = (sv+cv+2.0f)*0.25f; // in range [0,1]
    output[xy] = t*plasmaCol+(1.0f-t)*backgCol;
}