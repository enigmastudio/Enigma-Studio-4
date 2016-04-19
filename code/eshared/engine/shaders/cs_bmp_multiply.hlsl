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
SamplerState        ss:     register(s0);

cbuffer Params : register(b0)
{
    uint    count:      packoffset(c0.x);
    uint    mode:       packoffset(c1.x);
    float   angleVal:   packoffset(c2.x);
    float2  zoomVal:    packoffset(c3.x);
    float2  scrollVal:  packoffset(c4.x);
};

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint x = threadId.x;
    uint y = threadId.y;
    uint2 xy = int2(x, y);
    uint w, h;
    output.GetDimensions(w, h);
    uint2 wh = uint2(w, h);

    float3 res = input[xy].rgb;

    for (uint i=1; i<count+1; i++)
    {
        // roto zoom
        float s, c;
        sincos(angleVal*eTWOPI*i, s, c);
        float2 zoom = pow(float2(0.05f, 0.05f), zoomVal+(zoomVal-1.0f)*i-1.0f);
        float2 scroll = (scrollVal+(scrollVal-0.5f)*i)*wh;
        float u = ((-0.5f*w+x)*c+(0.5f*h-y)*s)*zoom.x+scroll.x;
        float v = ((-0.5f*w+x)*s-(0.5f*h-y)*c)*zoom.y+scroll.y;

        // merge
        float3 col = input.SampleLevel(ss, float2(u, v)/wh, 0.0f).rgb;

        if (mode == 0) // add
            res += col;
        else if (mode == 1) // sub
            res -= col;
        else if (mode == 2) // mul
            res *= col;
        else if (mode == 3) // difference
            res = abs(res-col);
        else if (mode == 4) // average
            res = (res+col)*0.5f;
        else if (mode == 5) // minimum
            res = min(res, col);
        else if (mode == 6) // maximum
            res = max(res, col);
    }

    output[xy] = float4(res, 1.0f);
}