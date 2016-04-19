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

#include "../../../system/system.hpp"
#include "kdtree.hpp"
#include "attractor.hpp"

void eLsys3Attractor::set(const eEditMesh &mesh, eU32 axis, eF32 amount, eBool massDep, eBool normalize) {
	this->m_normalize = normalize;
	this->m_massDependent = massDep;
	m_mesh = &mesh;
	m_axis = axis;
	m_amount = amount;
}

void eLsys3Attractor::applyAttraction(const eVector3& pos, eQuat &rotation, eF32 mass) {
	eVector3 normal = (m_mesh->getBoundingBox().getCenter() - pos);
	if(m_normalize)
		normal.normalize();
	eVector3 look = rotation.getVector(m_axis);

	eF32 cosAngleAttrLen = look * normal;
	eF32 cosAngleSqr = (cosAngleAttrLen * cosAngleAttrLen / normal.sqrLength());
    eF32 sinAngleSqr = 1.0f - cosAngleSqr;
	eF32 amount = (1.0f - (1.0f / (1.0f + m_amount)));
	if(m_massDependent)
		amount *= mass;
//	rotation.lerpAlongShortestArc(look, normal, amount);
	// |side| = sin(alpha)
	eVector3 side = look^normal;
	eF32 sideLen = side.length();
	if(sideLen > eALMOST_ZERO) {
		side /= sideLen;
		eQuat approtation(side, ePI * 0.5f * sinAngleSqr * amount);
		rotation = approtation * rotation;
	}
/**/
}
