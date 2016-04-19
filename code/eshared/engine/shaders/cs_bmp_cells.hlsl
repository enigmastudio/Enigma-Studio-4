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
    uint    numPoints:  packoffset(c2.x);
    float   regularity: packoffset(c3.x);
    uint    patternSel: packoffset(c4.x);
    uint    seed:       packoffset(c5.x);
    float   amplify:    packoffset(c6.x);
    float   gamma:      packoffset(c7.x);
    float4  col0:       packoffset(c8.x);
    float4  col1:       packoffset(c9.x);
};

cbuffer PointData : register(b2)
{
    float4  regPoints[1024];
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

    //[
    float2 points[2048] = (float2[2048])regPoints; // read agressively packed
    //]
    uint xo = x*numPoints/w+numPoints;
    uint yo = y*numPoints/h+numPoints;
    float xp = (float)x/(float)w;
    float yp = (float)y/(float)h;
    float maxDist = 0.0f;
    float minDist = w*h;
    float nextMinDist = w*h;

    for (int i=-1; i<2; i++)
    {
        int io = ((yo+i)%numPoints)*numPoints;

        for (int j=-1; j<2; j++)
        {
            float2 cellPos = points[((xo+j)%numPoints)+io];

            // make cells tileable
            if (j == -1 && x*numPoints < w)
                cellPos.x--;
            else if (j == 1 && x*numPoints >= w*(numPoints-1))
                cellPos.x++;

            if (i == 1 && y*numPoints >= h*(numPoints-1))
                cellPos.y++;
            else if (i == -1 && y*numPoints < h)
                cellPos.y--;

            // calculate the two shortest distances
            float dist = length(float2(xp, yp)-cellPos)*amplify;

            if (dist < minDist)
            {
                nextMinDist = minDist;
                minDist = dist;
            }
            else if (dist < nextMinDist)
                nextMinDist = dist;

            if (dist > maxDist)
                maxDist = dist;
        }
    }

    // set pixel intensity based on
    // calculated distances and pattern
    float intensity = 0.0f;

    if (patternSel == 0) // stone
        intensity = 1.0f-(nextMinDist-minDist)*(float)numPoints;
    else if (patternSel == 1) // cobweb
        intensity = 1.0f-minDist*(float)numPoints;

    output[xy] = lerp(col0, col1, saturate(intensity*gamma));
}