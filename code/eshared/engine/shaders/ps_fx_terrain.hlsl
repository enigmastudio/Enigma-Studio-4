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

Texture2D s_texColor : register(t0);
Texture2D s_texPosG : register(t2);
Texture2D s_texNoise : register(t4);
Texture2D s_texTerrain : register(t5);
Texture2D s_texGrass : register(t6);
Texture2D s_texEarth : register(t7);
Texture2D s_texRock : register(t8);
Texture2D s_texFoam : register(t9);

SamplerState s_samColor : register(s0);
SamplerState s_samPosG : register(s2);
SamplerState s_samNoise : register(s4);
SamplerState s_samTerrain : register(s5);
SamplerState s_samGrass : register(s6);
SamplerState s_samEarth : register(s7);
SamplerState s_samRock : register(s8);
SamplerState s_samFoam : register(s9);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float4          c_camPosition;
	float4          c_camFov;
    float4          c_sunPosition;
    float4          c_sunColor;
    float4          c_colSnow;
    float4          c_colRock;
    float4          c_colGrass;
    float4          c_colEarth;
    float4          c_terrainScale;
    float4          c_noiseScale;
    float4          c_skyColor;
    float4          c_planetCenter;
	float4			c_altitudeMax;
	float4			c_altitudeMin;
	float4			c_altitudeRel;
	float4			c_slopeMax;
	float4			c_slopeMin;
	float4			c_slopeRel;
	float4			c_skewAmo;
	float4			c_skewAzim;

	float4			c_uu;
	float4			c_vv;
	float4			c_ww;

    float           c_camRange;
    float           c_fogAmount;
    float           c_shadowAmount;
    float           c_planetRadius;
    float           c_terrainWarping;
    float           c_time;
    float           c_waterLevel;
    float           c_waterAmplitude;
    float           c_waterFrequency;
    float           c_waterShininess;

    float           c_skyLevel;
    float           c_skyHorizon;
    float           c_cloudScale;
    float           c_cloudTreshold;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float densityTerrainPlanar(float3 pos)
{
    float hight = pos.y;
    float density = -hight;

    density += s_texTerrain.SampleLevel(s_samTerrain, pos.xz / c_terrainScale.xz, 0.0).x * c_terrainScale.y;
    return density;
}
/*
float densityTerrainPlanarComplex(float3 pos)
{
    float hight = pos.y;
    float density = -hight;

    float2 warp = s_texNoise.SampleLevel(s_samNoise, pos.xz * c_noiseScale.xz * 4.123, 0.0).xy * c_terrainWarping;

    density += s_texTerrain.SampleLevel(s_samTerrain, pos.xz / c_terrainScale.xz, 0.0).x * c_terrainScale.y;
    density += s_texNoise.SampleLevel(s_samNoise, (pos.xz + warp) * c_noiseScale.xz, 0.0).x * c_noiseScale.y;
    density += noise2d(pos.xz).x * 0.002;

    return density;
}*/

float densityWaterPlanar(float3 pos)
{
    float hight = pos.y;
    float density = c_waterLevel - hight;

    // wave amplitude scale factors
    const float3 scale = float3(0.05, 0.04, 0.001);

    const float3 spx = float3(1.0,  1.6,  6.6);
    const float3 stx = float3(0.4, -0.4, -1.0);
    const float3 spz = float3(1.0,  1.7,  2.7);
    const float3 stz = float3(0.6, -0.6,  1.176);

    float3 x = spx * pos.x * c_waterFrequency + stx * c_time;
    float3 z = spz * pos.z * c_waterFrequency + stz * c_time;

    // wave amplitudes
    float3 amplitude = float3(
        snoise(float2(x.x, z.x)),
        snoise(float2(x.y, z.y)),
        snoise(float2(x.z, z.z)));
/*
    // wave amplitudes
    float3 amplitude = float3(
        snoise(float2(pos.x       + time * 0.4, pos.z       + time * 0.6)),
        snoise(float2(pos.x * 1.6 - time * 0.4, pos.z * 1.7 - time * 0.6)),
        snoise(float2(pos.x * 6.6 - time,       pos.z * 2.7 + time * 1.176)));
*/
    density += dot(amplitude, scale * c_waterAmplitude);
            
// Wind force in x and z axes.
float2 wind = {-0.3f, 0.7f};

density += s_texNoise.SampleLevel(s_samNoise, pos.xz * c_noiseScale.xz * 4.123 + c_time * 0.1f * wind, 0.0).x * c_terrainWarping;

    return density;
}

float4 densityCloud(float3 pos)
{
    const float4 scale = float4(0.5, 0.25, 0.125, 0.0625);

    float2 dir = normalize(pos-c_sunPosition.xyz).xz;

    float2 p = pos.xz;

    p.x += (c_time * 0.1);

	float4 n;
    n.w = dot(float4(snoise(p), snoise(p * 2.0), snoise(p * 8.0), snoise(p * 32.0)), scale);
	//n.w = noise(p * 1.0 ) * 0.5;
	//n.w += noise(p * 2.0 ) * 0.25;
	//n.w += noise(p * 8.0 ) * 0.125;
	//n.w += noise(p * 32.0 ) * 0.0625;

    p += dir;
    n.x = dot(float4(snoise(p), snoise(p * 2.0), snoise(p * 8.0), snoise(p * 32.0)), scale);
	//n.x = noise(p * 1.0 ) * 0.5;
	//n.x += noise(p * 2.0 ) * 0.25;
	//n.x += noise(p * 8.0 ) * 0.125;
	//n.x += noise(p * 32.0 ) * 0.0625;

    p += dir;
    n.y = dot(float4(snoise(p), snoise(p * 2.0), snoise(p * 8.0), snoise(p * 32.0)), scale);
	//n.y = noise(p * 1.0 ) * 0.5;
	//n.y += noise(p * 2.0 ) * 0.25;
	//n.y += noise(p * 8.0 ) * 0.125;
	//n.y += noise(p * 32.0 ) * 0.0625;

    p += dir;
    n.z = dot(float4(snoise(p), snoise(p * 2.0), snoise(p * 8.0), snoise(p * 32.0)), scale);
	//n.z = noise(p * 1.0 ) * 0.5;
	//n.z += noise(p * 2.0 ) * 0.25;
	//n.z += noise(p * 8.0 ) * 0.125;
	//n.z += noise(p * 32.0 ) * 0.0625;

	return n;
}

#define densityTerra densityTerrainPlanar
#define densityWater densityWaterPlanar

static const float2 GRADIENT_DELTA = float2(0.015,0);
static const float3 GRAD_x = GRADIENT_DELTA.xyy;
static const float3 GRAD_y = GRADIENT_DELTA.yxy;
static const float3 GRAD_z = GRADIENT_DELTA.yyx;

float3 getNormal( const float3 p )
{
	float3 grad;  

    float s = densityTerra(p);

    grad = float3(densityTerra(p - GRAD_x),
                  densityTerra(p - GRAD_y),
                  densityTerra(p - GRAD_z));

    return normalize(grad-s);
}

float3 normalFast(float3 p)
{
    float3 grad;

	float s = densityWater(p);

    grad = float3(densityWater(p - GRAD_x),
				  densityWater(p - GRAD_y),
				  densityWater(p - GRAD_z));

    return normalize(grad-s);
}

float random(in float a, in float b) 
{
    float f = (cos(dot(float2(a,b) ,float2(12.9898,78.233))) * 43758.5453);
    return frac(f);
}

// A noise function...
float NoiseNonLinear(in float x, in float y) 
{
    float i = floor(x), j = floor(y);
    float u = x-i, v = y-j;

    float du = 30.*u*u*(u*(u-2.)+1.);
    float dv = 30.*v*v*(v*(v-2.)+1.);
    u=u*u*u*(u*(u*6.-15.)+10.);
    v=v*v*v*(v*(v*6.-15.)+10.);

    float a = random( i, j );
    float b = random( i+1., j);
    float c = random( i, j+1.);
    float d = random( i+1., j+1.);
    float k0 = a;
    float k1 = b-a;
    float k2 = c-a;
    float k3 = a-b-c+d;
    return (k0 + k1*u + k2*v + k3*u*v);
}

float2 dddd(float3 pos)
{
    float2 dterra = float2(densityTerra(pos), 1);
    float2 dwater = float2(densityWater(pos), 0);
    return (dwater.x > dterra.x) ? dwater : dterra;
}

static const float MIN_RAYMARCH_DELTA = 0.01;

float castRay2(const float3 ro, const float3 rd)
{
    float lh = 0.0f;
	float3 p;
	float h;
	float t = 0.0;
    float oldt = 0.0;
    float res = 0;

    while( true )
    {
		if (t > c_camRange)
			break;

        float3 p = ro + rd * oldt;

		h = densityTerra(p);

        if( h > 0 )
        {
            res = t + (oldt-t)*abs(lh)/(h-lh);
            break;
        }
     
        t = oldt;

        // this is the sphere tracing increment t by the sphere radius
        oldt += max(-h, MIN_RAYMARCH_DELTA);

        lh = h;
    }

    if (res)
    {
        float3 p = ro + rd * t;
        float3 delt = rd * MIN_RAYMARCH_DELTA;

        // brute force
        while( true )
        {
			// do not step more than we have already done so
		    if (t > oldt)
			    break;

            t += MIN_RAYMARCH_DELTA;

            p += delt;

        	h = densityTerra(p);

            if( h > 0 )
                // interpolate the density
                return (t - MIN_RAYMARCH_DELTA + MIN_RAYMARCH_DELTA*abs(lh)/(h-lh));

            lh = h;
        }
    }

    return res;
}

float castShadowRay( const float3 ro, const float3 rd )
{
    float delt = 0.01f;
	float t = delt;

    while( true )
    {
		if (t > c_camRange)
			break;

        float3 p = ro + rd * t;
        float h = densityTerra(p);

        if( h > 0 )
            return t;

        // this is the sphere tracing increment t by the sphere radius
        t += max(-h, 0.01);
    }
    return 1.0f;
}

float3 getSky( const float3 rd, bool fog, out float dd)
{
    dd = 0;

    float sunAmount = max( dot( rd, normalize(c_sunPosition.xyz) ), 0.0 );
	float3 col = (float3(1,1,1) - c_skyColor.xyz) * rd.y + c_skyColor.xyz;
    float3 skyColor = lerp( col, c_sunColor.xyz, sunAmount );

    float sunPerc = pow(sunAmount, 190);
	
	float3 suncolor = lerp(c_sunColor.xyz, float3(1.0, 1.0, 1.0) * 4.0, 0.1);

	skyColor = lerp(skyColor, suncolor, sunPerc);

    if (fog)
        return float4(skyColor,1);

/*
    float sunAmount = max( dot( rd, normalize(c_sunPosition.xyz) ), 0.0 );

    float sunPerc = pow(sunAmount, 90);
	
	float3 suncolor = lerp(c_sunColor.xyz, float3(1.0, 1.0, 1.0) * 4.0, 0.1);

	float3 skyColor = lerp(c_skyColor.xyz, suncolor, sunPerc);
*/    

    // sky plane
    float4 n = float4(0, 1, 0, c_skyLevel);

    float d = dot(n.xyz, rd);
    float dist = (dot(n.xyz, float3(c_camPosition.x,-c_camPosition.y,c_camPosition.z)) + n.w) / d;
    if (dist > 0)
    {
        float3 pos = c_camPosition.xyz + rd * dist;

        float4 de = densityCloud(pos);

        // remaping: scale, threshold
        de = saturate(de * c_cloudTreshold + c_cloudScale);
        float alpha = de.w; 

        float4 ray = float4(0.0, 0.3, 0.6, 0.9);

        de -= ray;
        de = max(de, float4(0,0,0,0));

        // raymarch the clouds in one instruction :)
        float res = dot(de, float4(0.25,0.25,0.25,0.25));

        float3 cloudCol2 = float3(0.976,0.956,0.882);
        float3 cloudCol1 = float3(0.121,0.239,0.349);

        //skyColor = lerp(skyColor, lerp(cloudCol2, cloudCol1, res), alpha*pow(d,c_skyHorizon*c_skyHorizon));
        
        //skyColor = skyColor*(1-alpha) + lerp(cloudCol2, cloudCol1, res) * (alpha);

        skyColor = lerp(skyColor, lerp(cloudCol2, cloudCol1, res), alpha*pow(d,c_skyHorizon*c_skyHorizon));

        //skyColor = lerp(skyColor, res, saturate(pow(d,c_skyHorizon)));

        dd= dist;
    }

    return float4(skyColor,1);
}

float3 applyFog( in float3  rgb,      // original color of the pixel
              in float dis,            // camera to point distance
              in float3  rayOri,   // camera position
              in float3  rayDir )  // camera to point vector
{
    float fogAmount = clamp((c_fogAmount * exp(-rayOri.y*.25) * (1.0-exp( -dis*rayDir.y*.25 ))/rayDir.y), 0.0, 1.0);
    fogAmount = clamp(fogAmount + (exp(-(45.0-dis)*0.08))-0.1, 0.0, 1.0);
    float d;
    return lerp( rgb, getSky(rayDir, true, d), fogAmount );
}

#define PI 3.1415

float get(float height, float3 norm, float elevMax, float elevMin, float elevRel, float slopeMax, float slopeMin, float slopeRel, float skewAzim, float skewAmo, bool doSkew)
{
	//factor for this material
	float factor = 1;

	//are we to do skewing ?
	if (doSkew)
    {
		//calculate 2D skew vector
        float2 sk = float2(cos(skewAzim), sin(skewAzim));

		//skew scale value
		float scale = dot(norm.xy, sk)*skewAmo;
			
		//adjust elevation limits
		elevMax += scale;
		elevMin += scale;
	}

	//if outside limit elevation AND release skip this one
	if (elevMax + elevRel < height)
		return 0;
	if (elevMin - elevRel > height)
		return 0;

    //height release?
	if (elevMax < height)
        factor = 1 - (height-elevMax)/elevRel;
	if (elevMin > height)
        factor = 1 - (elevMin-height)/elevRel;

	//now check the slopes...

	//slope release
	float srel = cos(PI/2-slopeRel);

	//calculate min and max slope
	float minslope = cos(slopeMin);
	float maxslope = cos(slopeMax);

	//reverse ?
	if (minslope > maxslope)
    {
		float t=maxslope;maxslope=minslope;minslope=t;
	}

	//this slope is not supported for this type
	if (norm.y < minslope-srel) return 0;
	if (norm.y > maxslope+srel) return 0;

	//slope release?
	if (maxslope < norm.y)
        factor *= 1 - (norm.y-maxslope)/srel;
	if (minslope > norm.y)
        factor *= 1 - (minslope-norm.y)/srel;

    return factor;
}

float3 getTerrain( const float3 ro, const float3 rd, const float dis)
{
    // intersection point
    float3 pos = ro + rd * dis;
    // normal at intersection
	float3 nor = getNormal(pos);

    // diffuse therm at intersection
    float diffuse = dot(normalize(c_sunPosition.xyz), nor);
    float3 mat;

	//shadowing
	//float shadow = min(castShadowRay(pos, c_sunDirection.xyz), c_shadowAmount)/c_shadowAmount;
	float shadow = clamp(castShadowRay(pos, normalize(c_sunPosition.xyz)),0,1);

	//the skew denominator
	float skewDenom=nor.x*nor.x + nor.z*nor.z;

	//are we to do skewing... if it is a flat terrain there is no need
	bool doSkew = false;

	//if skew denominator is "almost" 0 then terrain is flat
	if (skewDenom > 0.00001) {
		//turn the skewing on and calculate the denominator
		doSkew=true;
		skewDenom=1.0/sqrt(skewDenom);
	}   

	float4 factor = float4(
		get(pos.y, nor,  c_altitudeMax.x, c_altitudeMin.x, c_altitudeRel.x, c_slopeMax.x, c_slopeMin.x, c_slopeRel.x, c_skewAzim.x, c_skewAmo.x * skewDenom, doSkew),
		get(pos.y, nor,  c_altitudeMax.y, c_altitudeMin.y, c_altitudeRel.y, c_slopeMax.y, c_slopeMin.y, c_slopeRel.y, c_skewAzim.y, c_skewAmo.y * skewDenom, doSkew),
		get(pos.y, nor,  c_altitudeMax.z, c_altitudeMin.z, c_altitudeRel.z, c_slopeMax.z, c_slopeMin.z, c_slopeRel.z, c_skewAzim.z, c_skewAmo.z * skewDenom, doSkew),
		get(pos.y, nor,  c_altitudeMax.w, c_altitudeMin.w, c_altitudeRel.w, c_slopeMax.w, c_slopeMin.w, c_slopeRel.w, c_skewAzim.w, c_skewAmo.w * skewDenom, doSkew));


	float totalFactor = dot(factor, float4(1,1,1,1));

	//factor = factor / totalFactor;

    mat = c_colSnow  * factor.w +
          c_colRock  * factor.z +
          c_colGrass * factor.y +
          c_colEarth * factor.x;

/*
        //Ground texture...
        float f = NoiseNonLinear(pos.x*14.0, pos.z*14.0)*0.4+0.3;
        mat = c_colEarth.xyz * f * s_texEarth.SampleLevel(s_samEarth, pos.xz / c_terrainScale.xz, 0.0).xyz;

		// slope of the terrain
		// dot(nor, upvec) == (nor.x * 0 + nor.y * 1 + nor.z * 0) == nor.y = cos(a)
		
		// add stone grey to steep places
		mat = lerp(mat, c_colRock.xyz * f * s_texRock.SampleLevel(s_samRock, pos.xz / c_terrainScale.xz, 0.0).xyz, 1-nor.y);

        // add snow on not too steep slopes (1 flat, 0 vertical)
		if (nor.y > .6)
        {
            float3 m = c_colSnow * clamp(NoiseNonLinear(pos.x*8.0, pos.z*8.0), 0.0, .9); 
            mat = lerp(mat, m, (nor.y-.6));
        }

        // add grass on not too steep slopes (1 flat, 0 vertical)
		if (nor.y > .8)
        {
            float3 m = c_colGrass.xyz * clamp(NoiseNonLinear(pos.x*8.0, pos.z*8.0), 0.0, .9) * s_texGrass.SampleLevel(s_samGrass, pos.xz / c_terrainScale.xz, 0.0).xyz; 
            mat = lerp(mat, m, (nor.y-.8));
        }
*/
				// difuse  			                  // ambient
    mat = mat * (c_sunColor.xyz*diffuse*shadow) + float3(0.05,0.05,0.01);

	return mat;
}

float fresnelTerm(float3 rd, float3 normal)
{
    // This value modifies current fresnel term. If you want to weaken
    // reflections use bigger value. If you want to empasize them use
    // value smaller then 0. Default is 0.0f.
    float refractionStrength = 0.0f;

    // R0 is a constant related to the index of refraction (IOR).
    // It should be computed on the CPU and passed to the shader.
    float R0 = 0.5f;

	float angle = 1.0f - saturate(abs(dot(normal, rd)));
	float fresnel = angle * angle;
	fresnel = fresnel * fresnel;
	fresnel = fresnel * angle;
	return saturate(fresnel * (1.0f - saturate(R0)) + R0 - refractionStrength);
}

float3 getWater( const float3 ro, const float3 rd, const float dis)
{
    // The smaller this value is, the more soft the transition between
    // shore and water. If you want hard edges use very big value.
    // Default is 1.0f.
    const float shoreHardness = 1.0f;

    // water surface point
    float3 pos = ro + rd * dis;

    // normal at surface point
    float3 normal = normalFast(pos);

    // Direction of the light
    float3 ld = normalize(c_sunPosition.xyz - pos);

    float fresnel = fresnelTerm(rd, normal);

    // get reflection
	float3 reflection;

	float3 red = reflect(rd, normal);

    float t;

	if (t = castRay2(pos, red))
		reflection = getTerrain(pos, red, t);
	else
		reflection = getSky(red, false, t);

    // Colour of the water surface
    const float3 depthColour = {0.0078f, 0.5176f, 0.7f};
    // Colour of the water depth
    const float3 bigDepthColour = {0.0039f, 0.00196f, 0.145f};
    // The rate at which colors die away in the water
    const float3 extinction = {7.0f, 30.0f, 40.0f};

    // How fast will colours fade out. You can also think about this
    // values as how clear water is. Therefore use smaller values (eg. 0.05f)
    // to have crystal clear water and bigger to achieve "muddy" water.
    const float fadeSpeed = 0.15f;

    // Water transparency along eye vector.
    const float visibility = 0.2f;
    const float sunScale = 3.0f;

    float3 color = reflection * (1-fresnel);

    float3 refraction;

    float3 rfd = refract(rd, normal, 1.33f);

    // check if our refraction vector is a null vector
    if (any(rfd))
    {
        refraction = reflection;

        if (t = castRay2(pos, rfd))
        {
            refraction = getTerrain(pos, rfd, t);

		    float3 depthN = t * fadeSpeed;
		    float3 waterCol = saturate(length(c_sunColor.xyz) / sunScale);
		    refraction = lerp(lerp(refraction, depthColour * waterCol, saturate(depthN / visibility)),
						    bigDepthColour * waterCol, saturate(/*depth2*/t / extinction));
        }

        color = lerp(refraction, reflection, fresnel);
    }
            
    float dotSpec = saturate(dot(reflect(ld,normal), rd) * 0.5f + 0.5f);
    float3 specular = (1.0f - fresnel) * pow(dotSpec, 112.0f) * (c_waterShininess * 1.8f + 0.2f) * c_sunColor.xyz;
    //specular += specular * 25 * saturate(c_waterShininess - 0.05f) * c_sunColor.xyz;

	color = saturate(color + specular);
            
    if (any(rfd))
		color = lerp(refraction, color, saturate(t * shoreHardness));

    return color;
}

/* spere tracing version */
float3 castRay(const float3 ro, const float3 rd, out float outt)
{
    float lh = 0.0f;
	float h, t = 0.0;
    float oldt = 0.0;
    bool hit = false;
    float terraHit = 0;
    float resT = 0;
    float3 col;

    while( true )
    {
		if (t > c_camRange)
			break;

        float3 p = ro + rd * oldt;

        float2 dens = dddd(p);

		h = dens.x;

        if( h > 0 )
        {
            resT = t + (oldt-t)*abs(lh)/(h-lh);
            terraHit = dens.y;
            hit = true;
            break;
        }
     
        t = oldt;

        // this is the sphere tracing increment t by the sphere radius
        oldt += max(-h, MIN_RAYMARCH_DELTA);

        lh = h;
    }

    if (hit)
    {
        float3 p = ro + rd * t;
        float3 delt = rd * MIN_RAYMARCH_DELTA;

        // brute force
        while( true )
        {
			// do not step more than we have already done so
		    if (t > oldt)
			    break;
                    
        	t += MIN_RAYMARCH_DELTA;

            p += delt;

            if (terraHit)
        		h = densityTerra(p);
            else
                h = densityWater(p);

            if( h > 0 )
            {   
                // interpolate the density
                resT = t - MIN_RAYMARCH_DELTA + MIN_RAYMARCH_DELTA*abs(lh)/(h-lh);
                break;
            }

            lh = h;
        }
    }

	if (hit)
	{
		if (terraHit)
			col = getTerrain(ro, rd, resT);
		else
			col = getWater(ro, rd, resT);

		col = applyFog(col, resT, ro, rd);
    }
	else
	{
		col = getSky(rd, false, resT);
	}

    outt = resT;

    return col;
}

float intersectPlane(float3 ro, float3 rd, float4 plane)
{
    float t = (-plane.w - dot(ro, plane.xyz)) / dot(rd, plane.xyz);
    return t;
}

float4 main(const ShaderInput input) : SV_Target0
{
	float2 p = 2.0 * input.texCoord - 1.0;
	p.x *= c_camFov.z;

    float3 rd = normalize(p.x*c_uu - p.y*c_vv + c_camFov.x*c_ww);

    float depth1 = 0;
    float3 col = castRay(c_camPosition.xyz, rd, depth1);

    float depth2 = s_texPosG.SampleLevel(s_samPosG, input.texCoord, 0.0).z;

    depth1 = (depth1==0) ? 1000000 : depth1;

    if (depth2 > 0 && depth1 > depth2)
        return float4(s_texColor.SampleLevel(s_samColor, input.texCoord, 0.0).xyz, 1.0f);
    else
        return float4(col, 1.0f);
}


// 3d clouds
/*
float svnoise(float3 v)
{
	float n = snoise(v);
	n += snoise(v * 2.0 ) * 0.5;    
	n += snoise(v * 8.0 ) * 0.25;   
	n += snoise(v * 32.0 ) * 0.125; 
	n += snoise(v * 64.0 ) * 0.0625;
 
	return ( n > 0.6 ) ? 1.0 : 0.0;
}

float4 main(const ShaderInput input) : SV_Target0
{
    float2 resolution;
    s_texPosG.GetDimensions(resolution.x, resolution.y);

	float2 p = 1.0 - 2.0 * input.texCoord;
	p.x *= resolution.x/resolution.y;

    float3 ww = normalize(c_camLookAt.xyz - c_camPosition.xyz);
    float3 uu = normalize(cross(c_camUpVec.xyz, ww));
    float3 vv = normalize(cross(ww, uu));
    float3 rd = normalize(p.x*uu + p.y*vv + 1.5*ww);

    float g_absorption = 0.5;
    float g_lightIntensity = 1;

	const float maxDist = sqrt(3.0);
 
	const int numSamples = 128;
	const float scale = maxDist / float(numSamples);
 
	const int numLightSamples = 32;
	const float lscale = maxDist / float(numLightSamples);
 
	float3 pos = c_camPosition.xyz;
	float3 eyeDir = rd*scale;
 
	float T = 1.0;
	float3 Lo = float3(0,0,0);
 
	for (int i=0; i < numSamples; ++i)
	{
		float density = svnoise(pos);
 
		if (density > 0.0)
		{
			T *= 1.0 - density*scale*g_absorption;
			if (T <= 0.01)
				break;
 
			float3 lightDir = normalize(c_sunPosition.xyz - pos)*lscale;
 
			float Tl = 1.0;
			float3 lpos = pos + lightDir;
 
			for (int s=0; s < numLightSamples; ++s)
			{
				float ld = svnoise(lpos);
				Tl *= 1.0 - ld*lscale*g_absorption;
 
				if (Tl <= 0.01)
					break;
 
				lpos += lightDir;
			}
 
			float3 Li = g_lightIntensity*Tl;
 
			Lo += Li*T*density*scale;
		}
 
		pos += eyeDir;
	}
 
    return float4(Lo, 1.0f-T);
}
*/
// plane
/*
float4 main(const ShaderInput input) : SV_Target0
{
    float2 resolution;
    s_texPosG.GetDimensions(resolution.x, resolution.y);

	float2 p = 1.0 - 2.0 * input.texCoord;
	p.x *= resolution.x/resolution.y;

    float3 ww = normalize(c_camLookAt.xyz - c_camPosition.xyz);
    float3 uu = normalize(cross(c_camUpVec.xyz, ww));
    float3 vv = normalize(cross(ww, uu));
    float3 rd = normalize(p.x*uu + p.y*vv + 1.5*ww);

    return float4(getSky(rd, false), 1.0);
}
*/

//sky

/*
float4 main(const ShaderInput input) : SV_Target0
{
    float2 resolution;
    s_texPosG.GetDimensions(resolution.x, resolution.y);

	float2 p = 1.0 - 2.0 * input.texCoord;
	p.x *= resolution.x/resolution.y;

    float3 ww = normalize(c_camLookAt.xyz - c_camPosition.xyz);
    float3 uu = normalize(cross(c_camUpVec.xyz, ww));
    float3 vv = normalize(cross(ww, uu));
    float3 rd = normalize(p.x*uu + p.y*vv + 1.5*ww);

    float g_absorption = 0.5;
    float g_lightIntensity = 1;

    float4 n = float4(0,1,0,3);

    float d = dot(n.xyz, rd);
    float dist = (dot(n.xyz, c_camPosition.xyz) + n.w) / d;
    if (dist > 0)
    {
        float3 pos = c_camPosition.xyz + rd * dist;

        float4 de = densityCloud(pos);

        float a = c_noiseScale.x;
        float b = c_noiseScale.y;

        // remaping: scale, bias, threshold
        de = saturate(de * a + b);
        float alpha = de.w; 

        float4 ray = float4(0.0, 0.3, 0.6, 0.9);

        de -= ray;
        de = max(de, float4(0,0,0,0));

        float res = dot(de, float4(0.25,0.25,0.25,0.25));

        float3 col1 = float3(0.7,0.7,0.7);
        float3 col2 = float3(0.3,0.3,0.3);

        float3 col = lerp(lerp(col2,col1,res),getSky(rd), alpha);

        return float4(col,1);
    }
 
    return float4(1,0,0,0);
}
*/