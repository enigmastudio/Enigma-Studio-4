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

#ifndef REWRITESYSTEM_HPP
#define REWRITESYSTEM_HPP

#include "../../../eshared.hpp"

#define LSYS3_NUM_PARAMS 8
#define LSYS3_RULE_SEPARATOR ';'

class eRewriteSystem {
public:
	typedef struct SymInstance {
		eU32 symbol;
		eF32 params[LSYS3_NUM_PARAMS];
	};

	typedef struct Rule {
		eU32 conditionStart;
		eU32 conditionEnd;
		eU32 prodStart;
		eU32 prodEnd;
	};

	eString			m_ruleString;
	eArray<Rule>	m_rules[256];
	eArray<eF32> ruleProbabilities;
	eArray<SymInstance>		m_buf0, m_buf1;

	static SymInstance NewSymInstance(eU32 c, const eF32* params, eU32 cnt);
	void readRules(const eString& ruleString);
	void apply(const SymInstance& axiom, eArray<SymInstance>& result, eU32 iterations, eU32 randomSeed);

	// calculates the expression in the string
	// returns the position after second number
	static eS32 calculateTerm(const eString& s, int start, int end, const eF32* params, eF32& result);
	static eS32 readValInstanciation(const eString& s, int start, int end, eF32* params, const eF32* prevParams);
	static eString toString(eArray<SymInstance>& r, eBool withParams);
};

#endif // REWRITESYSTEM_HPP