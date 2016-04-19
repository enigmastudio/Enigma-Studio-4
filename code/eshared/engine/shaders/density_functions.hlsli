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

float densityTerrainPlanar(float3 pos)
{
    float hight = pos.y;
    float density = -hight;

    density += s_texTerrain.SampleLevel(s_samTerrain, pos.xz / c_terrainScale.xz, 0.0).x * c_terrainScale.y;
    return density;
}

float densityTerrainPlanarComplex(float3 pos)
{
    float hight = pos.y;
    float density = -hight;

    float2 warp = s_texNoise.SampleLevel(s_samNoise, pos.xz * c_noiseScale.xz * 4.123, 0.0).xy * c_terrainWarping;

    density += s_texTerrain.SampleLevel(s_samTerrain, pos.xz / c_terrainScale.xz, 0.0).x * c_terrainScale.y;
    density += s_texNoise.SampleLevel(s_samNoise, (pos.xz + warp) * c_noiseScale.xz, 0.0).x * c_noiseScale.y;
    density += noise2d(pos.xz).x * 0.002;

    return density;
}

float densityTerrainSpherical(float3 pos)
{
    float hight = length(pos.xyz - c_planetCenter.xyz) - c_planetRadius;
    float density = -hight;

	//get spherical uv coordinates
	float2 uv;
	float3 Vp = normalize(pos - c_planetCenter.xyz);
/*
	uv.x = asin(Vp.x)/3.1415 + 0.5;
	uv.y = acos(Vp.y)/3.1415 + 0.5;
*/
	float3 Vn = float3(0,1,0);
	float3 Ve = float3(1,0,0);

	float phi = acos(-dot(Vn,Vp));

	uv.y = phi/3.1415;

	float theta = ( acos( dot( Vp, Ve ) / sin( phi )) ) / ( 2.0 * 3.1415);
	if ( dot( cross( Vn, Ve ), Vp ) > 0 )
		uv.x = theta;
	else
		uv.x = 1.0 - theta;

    density += s_texTerrain.SampleLevel(s_samTerrain, uv * c_terrainScale.xz, 0.0).x * c_terrainScale.y;
    return density;
}

float densityWaterSpherical(float3 pos)
{
    float hight = length(pos.xyz - c_planetCenter.xyz) - c_planetRadius;
    float density = c_waterLevel - hight;

    // wave amplitude scale factors
    const float3 scale = float3(0.05, 0.04, 0.001);

    const float3 spx = float3(1.0,  1.6,  6.6);
    const float3 stx = float3(0.4, -0.4, -1.0);
    const float3 spz = float3(1.0,  1.7,  2.7);
    const float3 stz = float3(0.6, -0.6,  1.176);

	float2 uv;
    float3 Vp = normalize(pos - c_planetCenter.xyz);
/*
	uv.x = asin(Vp.x)/3.1415 + 0.5;
	uv.y = asin(Vp.y)/3.1415 + 0.5;
*/
	float3 Vn = float3(0,1,0);
	float3 Ve = float3(1,0,0);

	float phi = acos(-dot(Vn,Vp));

	uv.y = phi/3.1415;

	float theta = ( acos( dot( Vp, Ve ) / sin( phi )) ) / ( 2.0 * 3.1415);
	if ( dot( cross( Vn, Ve ), Vp ) > 0 )
		uv.x = theta;
	else
		uv.x = 1.0 - theta;


    float3 x = spx * uv.x * c_waterFrequency + stx * c_time;
    float3 z = spz * uv.y * c_waterFrequency + stz * c_time;

    // wave amplitudes
    float3 amplitude = float3(
        snoise(float2(x.x, z.x)),
        snoise(float2(x.y, z.y)),
        snoise(float2(x.z, z.z)));

    density += dot(amplitude, scale * c_waterAmplitude);
            
// Wind force in x and z axes.
//float2 wind = {-0.3f, 0.7f};

density += s_texNoise.SampleLevel(s_samNoise, uv * c_noiseScale.xz * 4.123 /* + c_time * 0.1f * wind*/, 0.0).x * c_terrainWarping;

    return density;
}

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

float densityTerrainComplex(float3 pos)
{
    float hight = length(pos.xyz - c_planetCenter.xyz) - c_planetRadius;
    float density = -hight;

    float2 warp = s_texNoise.SampleLevel(s_samNoise, pos.xz * c_noiseScale.xz * 4.123, 0.0).xy * c_terrainWarping;

    density += s_texTerrain.SampleLevel(s_samTerrain, pos.xz / c_terrainScale.xz, 0.0).x * c_terrainScale.y;
    density += s_texNoise.SampleLevel(s_samNoise, (pos.xz + warp) * c_noiseScale.xz, 0.0).x * c_noiseScale.y;
    density += noise2d(pos.xz).x * 0.002;

    return density;
}

float2 getSphericalCoordinates(float3 pos)
{
	float2 uv;
    float3 Vp = normalize(pos);

    float3 Vn = float3(0,1,0);
	float3 Ve = float3(1,0,0);

	float phi = acos(-dot(Vn,Vp));

	uv.y = phi/3.1415;

	float theta = ( acos( dot( Vp, Ve ) / sin( phi )) ) / ( 2.0 * 3.1415);
	if ( dot( cross( Vn, Ve ), Vp ) > 0 )
		uv.x = theta;
	else
		uv.x = 1.0 - theta;

    return uv;
}

float4 densityCloudSpherical(float3 pos)
{
    const float4 scale = float4(0.5, 0.25, 0.125, 0.0625);

//    float2 dir = getSphericalCoordinates(pos.xyz - c_sunPosition.xyz); 
    
//  normalize(pos-c_sunPosition.xyz).xy;
/*
	dir.x = asin(dir.x)/3.1415;
	dir.y = asin(dir.y)/3.1415;
*/

    float2 uv = getSphericalCoordinates(pos - c_planetCenter.xyz);

    float2 dir = float2(0,0);

/*
	float2 uv;
    float3 Vp = normalize(pos - c_planetCenter.xyz);

    float3 Vn = float3(0,1,0);
	float3 Ve = float3(1,0,0);

	float phi = acos(-dot(Vn,Vp));

	uv.y = phi/3.1415;

	float theta = ( acos( dot( Vp, Ve ) / sin( phi )) ) / ( 2.0 * 3.1415);
	if ( dot( cross( Vn, Ve ), Vp ) > 0 )
		uv.x = theta;
	else
		uv.x = 1.0 - theta;
*/
    uv *= 10;
    dir *= 10;

/*
	float2 uv;
    float3 Vp = normalize(pos - c_planetCenter.xyz);
	uv.x = 10*asin(Vp.x)/3.1415 + 0.5;
	uv.y = 10*asin(Vp.y)/3.1415 + 0.5;
*/
	float4 n;
    n.w = dot(float4(snoise(uv), snoise(uv * 2.0), snoise(uv * 8.0), snoise(uv * 32.0)), scale);
	//n.w = noise(p * 1.0 ) * 0.5;
	//n.w += noise(p * 2.0 ) * 0.25;
	//n.w += noise(p * 8.0 ) * 0.125;
	//n.w += noise(p * 32.0 ) * 0.0625;

    uv += dir;
    n.x = dot(float4(snoise(uv), snoise(uv * 2.0), snoise(uv * 8.0), snoise(uv * 32.0)), scale);
	//n.x = noise(p * 1.0 ) * 0.5;
	//n.x += noise(p * 2.0 ) * 0.25;
	//n.x += noise(p * 8.0 ) * 0.125;
	//n.x += noise(p * 32.0 ) * 0.0625;

    uv += dir;
    n.y = dot(float4(snoise(uv), snoise(uv * 2.0), snoise(uv * 8.0), snoise(uv * 32.0)), scale);
	//n.y = noise(p * 1.0 ) * 0.5;
	//n.y += noise(p * 2.0 ) * 0.25;
	//n.y += noise(p * 8.0 ) * 0.125;
	//n.y += noise(p * 32.0 ) * 0.0625;

    uv += dir;
    n.z = dot(float4(snoise(uv), snoise(uv * 2.0), snoise(uv * 8.0), snoise(uv * 32.0)), scale);
	//n.z = noise(p * 1.0 ) * 0.5;
	//n.z += noise(p * 2.0 ) * 0.25;
	//n.z += noise(p * 8.0 ) * 0.125;
	//n.z += noise(p * 32.0 ) * 0.0625;

	return n;
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