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

#ifndef LSYS3_ATTRACTOR_HPP
#define LSYS3_ATTRACTOR_HPP

#include "../../../eshared.hpp"
#include "kdtree.hpp"

class eLsys3Attractor
{
public:

	void set(const eEditMesh &mesh, eU32 axis, eF32 amount, eBool massDep, eBool normalize);
	void applyAttraction(const eVector3& pos, eQuat &rotation, eF32 mass);

	eU32	m_axis;
	eF32	m_amount;
	const eEditMesh *	m_mesh;
	eBool	m_massDependent;
	eBool	m_normalize;
};


#endif // LSYS3_ATTRACTOR_HPP