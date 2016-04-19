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
#include "tinylsys3.hpp"
#include "rewritesystem.hpp"
#include "attractor.hpp"
#include "turtleinterpreter.hpp"

eLsys3TurtleInterpreter::eLsys3TurtleInterpreter() {
	rotationUnit = eVector3(1,1,1);
	scaleFak = 2.0f;
}

void eLsys3TurtleInterpreter::initializeState(TurtleState& state) {
	state.m_localPos = eVector3();
	state.m_localRot = eQuat();
};

void eLsys3TurtleInterpreter::interpret(eArray<eLSys3InterpreterNode>& result, eArray<eRewriteSystem::SymInstance>& s, const eVector3& rotUnit, eF32 decay) {
	TurtleState st;
	st.m_len = 1.0f;
	st.m_width = 1.0f;
	st.m_lenSum = 0.0f;
	st.m_localPos = eVector3();
	st.m_localRot = eQuat();
	st.m_material = 0;
	for(eU32 i = 0; i < LSYS3_NUM_PARAMS; i++)
		st.m_parameters[i] = 1.0f;
	this->rotationUnit = rotUnit;
	this->decay = decay;
	interpret(result, st, s);
}

// returns eTRUE if this state should be saved
eBool eLsys3TurtleInterpreter::interpretSymbol(eU32 symbol, eF32* params, TurtleState& state) {
	switch(symbol) {
		case ' ':  break;
		case '-':  state.rotate(0, -rotationUnit[0] * params[0]); break;
		case '+':  state.rotate(0,  rotationUnit[0] * params[0]); break;
		case '&':  state.rotate(1, -rotationUnit[1] * params[0]); break;
		case '^':  state.rotate(1,  rotationUnit[1] * params[0]); break;
		case '\\':  state.rotate(2, -rotationUnit[2] * params[0]); break;
		case '/':  state.rotate(2,  rotationUnit[2] * params[0]); break;
		case '<':  state.m_len /= scaleFak; break;
		case '>':  state.m_len *= scaleFak; break;
		case '!':  state.m_material++; break;
		case '#':  state.m_width = params[0]; break;
		default:
			if((symbol == 'F') || (symbol == 'G')) {
				eF32 stepLen = params[0] * state.m_len;
				state.move(stepLen);
				state.m_lenSum += stepLen;
				state.m_len *= this->decay;
				state.m_width *= this->decay;
				if(symbol == 'F') {
					// the current turtle position is relevant
					// copy parameters
					for(eU32 i = 0; i < LSYS3_NUM_PARAMS; i++)
						state.m_parameters[i] = params[i];
					return eTRUE;
				}
			}
		break;
	}
	return eFALSE;
}


void eLsys3TurtleInterpreter::interpret(eArray<eLSys3InterpreterNode>& result, const TurtleState& initialState, eArray<eRewriteSystem::SymInstance>& s) {
	result.clear();
	result.reserve(s.size());
	stateStack.clear();
	stateStack.reserve(s.size());

	eLSys3InterpreterNode curNode;
	curNode.parent = -1;
	curNode.state = initialState;
	result.append(curNode);
	initializeState(curNode.state);

	curNode.parent = 0;

	// interpret symbols
	for(eU32 i = 0; i < s.size(); i++) {
		switch(s[i].symbol) {
		case '[':
			stateStack.push(curNode);
			break;
		case ']':
			curNode = stateStack.pop();
			break;
		default:
			if(interpretSymbol(s[i].symbol, s[i].params, curNode.state)) {
				result.append(curNode);
				curNode.parent = result.size() - 1;
				initializeState(curNode.state);
			}
		}
	}

}

void eLsys3TurtleInterpreter::postProcess(eArray<eLSys3InterpreterNode>& results, eArray<eLsys3Attractor*>& attractors) {
	// initialize
	for(eU32 i = 0; i < results.size(); i++) 
		results[i].postProc.mass = 1.0f;
	if(results.size() == 0)
		return;

	// calculate mass sums
	for(eS32 i = results.size() - 1; i >= 0; i--) {
		eLSys3InterpreterNode& node = results[i];
		if(node.parent != -1) {
			eLsys3PostProcRec& r = results[i].postProc;
			eLsys3PostProcRec& rp = results[node.parent].postProc;
			rp.mass += r.mass;
		}
	}
	eF32 totalMass = results[0].postProc.mass;

	// now process all
	for(eU32 i = 0; i < results.size(); i++) {
		eLSys3InterpreterNode& node = results[i];
		eLsys3PostProcRec& r = results[i].postProc;

		r.mass /= totalMass;
		// calculate final rotation
		r.globalRot = ((node.parent == -1) ? eQuat()    : results[node.parent].postProc.globalRot) * node.state.m_localRot;

		eVector3 basePos = ((node.parent == -1) ? eVector3() : results[node.parent].postProc.globalPos);
		// apply forces
		for(eU32 a = 0; a < attractors.size(); a++) {
			eVector3 curPos = basePos + node.state.m_localPos * r.globalRot.conjugated();
			attractors[a]->applyAttraction(curPos, r.globalRot, r.mass);
		}
		// calculate final position
		r.globalPos = basePos + node.state.m_localPos * r.globalRot.conjugated().normalized();
	}

}

