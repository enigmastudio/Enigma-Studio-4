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
    float4  col0:    packoffset(c0.x);
    float4  col1:    packoffset(c1.x);
    float   percent: packoffset(c2.x);
    uint    seed:    packoffset(c3.x);
};

float random(float2 p)
{
    // we need irrationals for pseudo randomness.
    // most (all?) known transcendental numbers
    // will (generally) work.
    const float GELFOND = 23.1406926327792690f; // e^pi
    const float GELFOND_SCHNEIDER = 2.6651441426902251; // 2^sqrt(2)
    const float2 r = float2(GELFOND, GELFOND_SCHNEIDER);
    return frac(cos(fmod(123456789.0f, 1e-7f+256.0f*dot(p, r))));
}

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint x = threadId.x;
    uint y = threadId.y;
    uint2 xy = int2(x, y);
    uint w, h;
    output.GetDimensions(w, h);
    uint2 wh = uint2(w, h);

    float c = random(float2(xy)*float2(seed, seed)/float2(wh));
    float p = percent/100.0f;
    float t = c/p;

    output[xy] = (c < p ? lerp(col0, col1, t) : input[xy]);
}