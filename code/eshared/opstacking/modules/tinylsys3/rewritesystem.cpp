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

eRewriteSystem::SymInstance eRewriteSystem::NewSymInstance(eU32 c, const eF32* params, eU32 cnt) {
	SymInstance s;
	s.symbol = c;
	for(eU32 i = 0; i < LSYS3_NUM_PARAMS; i++)
		s.params[i] = (i < cnt) ? params[i] : 1.0f;
	return s;
}

void eRewriteSystem::readRules(const eString& ruleString) {
	// clear rules
	for(eU32 i = 0; i < 256; i++)
		m_rules[i].clear();

	m_ruleString = ruleString;

	eU32 pos = 0;
	// read rules
	while(pos < ruleString.length()) {
		Rule r;
		// read rule axiom symbol
		eU32 axSym = ruleString[pos++];
		r.conditionStart = pos;
		while(ruleString[pos] != LSYS3_RULE_SEPARATOR)
			pos++;
		r.conditionEnd = pos;
		r.prodStart = ++pos;
		while(ruleString[pos] != LSYS3_RULE_SEPARATOR)
			pos++;
		r.prodEnd = pos++;
		m_rules[axSym].append(r);
	}
};

void eRewriteSystem::apply(const eRewriteSystem::SymInstance& axiom, eArray<eRewriteSystem::SymInstance>& result, eU32 iterations, eU32 randomSeed) {
	eU32 localRanSeed = randomSeed;
	m_buf0.clear();
	m_buf1.clear();
	eArray<SymInstance>* prevProd = &m_buf0;
	eArray<SymInstance>* curProd  = &m_buf1;
	curProd->append(axiom);
	for(eU32 it = 0; it < iterations; it++) {
		eSwap(prevProd, curProd);
		curProd->clear();
		for(eU32 p = 0; p < prevProd->size(); p++) {
			SymInstance& curSym = (*prevProd)[p];
			eArray<Rule>& rules = m_rules[curSym.symbol];
			if(rules.size() == 0) {
				// copy symbol
				curProd->append(curSym);
			} else {
				// find rule
				ruleProbabilities.clear();
				eF32 probSum = 0.0f;
				for(eU32 i = 0; i < rules.size(); i++) {
					Rule& rule = rules[i];
					// calculate probability
					SymInstance test = NewSymInstance(0, nullptr, 0);
					eU32 pos = readValInstanciation(m_ruleString, rule.conditionStart, rule.conditionEnd, test.params, curSym.params);
					eF32 probability = test.params[0];
					ruleProbabilities.append(probability);
					probSum += probability;
				}

				if(probSum == 0.0f) {
					// no rule applies, just copy
					curProd->append(curSym);
				} else {
					// apply random rule
					eF32 r = eRandomF(localRanSeed) * probSum;
					eF32 p = 0.0f;
					eU32 ridx = 0;
					while(ridx < ruleProbabilities.size()) {
						p += ruleProbabilities[ridx];
						if(p > r) {
							// this rule will be applied
							Rule& rule = rules[ridx];
							eU32 pos = rule.prodStart;
							while(pos < rule.prodEnd) {
								curProd->append(NewSymInstance(m_ruleString[pos++], nullptr, 0));
								SymInstance& nsym = curProd->last();
								if((pos < rule.prodEnd - 1) && (m_ruleString[pos] == '('))
									pos = readValInstanciation(m_ruleString, pos, rule.prodEnd, nsym.params, curSym.params);
							}
							break;
						}
						ridx++;
					}
				}
			}
		}
	}
	// copy curProd to result
	result.append(*curProd);
}



// calculates the expression in the string
// returns the position after second number
eS32 eRewriteSystem::calculateTerm(const eString& s, int start, int end, const eF32* params, eF32& result) {
	int pos = start;
	eF32 tuple[] = {0,0,0,0,0,0,0,0,0,0};
	eU32 ops[] = {0,0,0,0,0,0,0,0,0,0};
	eU32 tpos = 0;
	eU32 opPos = 0;
	eU32 c;
	while((pos < end) && ((c = s[pos]) != ',') && (c != ')')) {
		if(c == '(') {
			pos = calculateTerm(s, pos + 1, end, params, tuple[tpos]);
			eASSERT(pos != -1);
			pos++;
		} else {
			eS32 oldPos = pos;
			// try to read constant
			eF32 number = 0;
			eF32 v = 1.0f;
			while(pos < end) {
				int cc = s[pos] - '0';
				if(cc == '.' - '0')
					v = 0.1f;
				else if((eU32)cc <= 9) {// is a number
						if(v >= 1.0f)
							number = number * 10.0f + cc;
						else {
							number += v * cc;
							v /= 10.0f;
						}
					tuple[tpos] = number;
				}
				else break;
				pos++;
			}
 			// try to read symbol
			for(eU32 i = 0; i < LSYS3_NUM_PARAMS; i++)
				if('a' + i == c) {
					tuple[tpos] = params[i];
					pos++;
				}
			if(pos == oldPos) {
				// is an operator
				pos++;  
				ops[opPos++] = c;	
				tpos++;
			};
		}
	}

	result = tuple[0];
	for(eU32 o = 0; o < opPos; o++) {
		eU32 op = ops[o];
		eF32 t0 = result;
		eF32 t1 = tuple[o + 1];
		switch(op) {
		case '+': result = t0 + t1; break;
		case '-': result = t0 - t1; break;
		case '*': result = t0 * t1; break;
		case '/': result = t0 / t1; break;
		case '<': result = (t0 < t1) ? 1.0f : 0.0f; break;
		case '=': result = (t0 == t1) ? 1.0f : 0.0f; break;
		case '>': result = (t0 > t1) ? 1.0f : 0.0f; break;
		case '_': result = (t0 <= t1) ? 1.0f : 0.0f; break;
		case '~': result = t0 * eSin(t1); break;
		case '#': result = t0 + eRandomF() * t1; break;
		case '^': 
			result = ePow(t0, t1); break;
		default:
			result = t0; // single term
		}
	}
	return pos;
}

eS32 eRewriteSystem::readValInstanciation(const eString& s, int start, int end, eF32* params, const eF32* prevParams) {
	int pos = start;
	int v = 0;

	if((pos >= end) || (s[pos] != '(')) {
		return pos;
	} else {
		pos++;
		while((pos < end) && (s[pos] != ')')) {
			if(s[pos] == ',') {
				pos++;
				continue;
			}
			pos = calculateTerm(s, pos, end, prevParams, params[v++]); 
			eASSERT(pos != -1);
		}
	}
	return pos + 1;
}

#ifdef eEDITOR
eString eRewriteSystem::toString(eArray<eRewriteSystem::SymInstance>& r, eBool withParams) {
	eString s = "";
	for(eU32 i = 0; i < r.size(); i++) {
		eString sym = "a";
		sym.at(0) = r[i].symbol;
		s += sym;
		if(withParams) {
			s += "(";
			for(eU32 k = 0; k < LSYS3_NUM_PARAMS; k++) {
				if(k != 0)
					s += ",";
				s += eFloatToStr(r[i].params[k]);
			}
			s += ")";
		}
	}
	return s;
}
#endif