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
    float2  startVal:   packoffset(c0.x);
    float2  endVal:     packoffset(c1.x);
    float   thickness:  packoffset(c2.x);
    float   decay:      packoffset(c3.x);
    float4  col:        packoffset(c4.x);
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

    float2 start = startVal*wh;
    float2 end = endVal*wh;
    float2 vecAb = end-start;
    float lineLen = length(vecAb);
    float invLineLenQ = 1.0f/(lineLen*lineLen);
    float u = ((x-start.x)*(end.x-start.x)+(y-start.y)*(end.y-start.y))*invLineLenQ;
    float alpha = 1.0f;

    float real_thickness = thickness * w / 256;
    float real_decay = decay * w / 256;

    if (u >= 0.0f && u <= 1.0f)
    {
        float2 intersection = start + u * vecAb;
        float distance = length(xy - intersection);
        float thicknessDist = distance - real_thickness;

        alpha = saturate(thicknessDist / real_decay);
    }

    float4 inCol = input[xy];
    output[xy] = (1.0f-alpha)*col+alpha*inCol;
}