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

#ifndef LSYS3_TURTLE_INTERPRETER_HPP
#define LSYS3_TURTLE_INTERPRETER_HPP

#include "../../../eshared.hpp"
#include "rewritesystem.hpp"
#include "attractor.hpp"

struct TurtleState {
	eVector3					m_localPos;
	eQuat						m_localRot;
	eF32                        m_len;
	eF32						m_width;
	eF32						m_lenSum;
	eU32						m_material;
	eF32						m_parameters[LSYS3_NUM_PARAMS];

	void move(eF32 units) {
		m_localPos[2] += units;
    }

    void rotate(eU32 axis, eF32 amount) {
		if(amount != 0.0) {
			this->m_localRot = eQuat(this->m_localRot.getVector(axis), amount) * this->m_localRot;
		}
    }
};

struct eLsys3PostProcRec {
	eF32		mass;
	eVector3	globalPos;
	eQuat		globalRot;
};


struct eLSys3InterpreterNode {
	eU32				parent;
	TurtleState			state;
	eLsys3PostProcRec	postProc;
};


class eLsys3TurtleInterpreter {
public:

	eVector3		rotationUnit;
	eF32			decay;
	eF32			scaleFak;
	eArray<eLSys3InterpreterNode>	stateStack;

	eLsys3TurtleInterpreter();
	void initializeState(TurtleState& state);
	void interpret(eArray<eLSys3InterpreterNode>& result, eArray<eRewriteSystem::SymInstance>& s, const eVector3& rotUnit, eF32 decay);
	// returns eTRUE if this state should be saved
	eBool interpretSymbol(eU32 symbol, eF32* params, TurtleState& state);
	void interpret(eArray<eLSys3InterpreterNode>& result, const TurtleState& initialState, eArray<eRewriteSystem::SymInstance>& s);
	void postProcess(eArray<eLSys3InterpreterNode>& results, eArray<eLsys3Attractor*>& attractors);
};

#endif // LSYS_INTERPRETER_HPP