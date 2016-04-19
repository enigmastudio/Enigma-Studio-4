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

#ifdef eEDITOR
#include <Windows.h>
#include <stdio.h>
#endif
#include "../../../eshared.hpp"
#include "kdtree.hpp"

eGenericKDTree::eGenericKDTree() {
	this->randomSeed = 12342;
}

void eGenericKDTree::clear() {
	m_leafs.clear();
}

void eGenericKDTree::add(const eVector3& position, const void* value) {
	Entry e;
	e.m_position = position;
	e.m_leaf = value;
	m_leafs.append(e);
}
void eGenericKDTree::remove(const void* val, Node& node) {
	ePROFILER_ZONE("KDTree Remove");
	for(eU32 i = node.m_leafStartIdx; i < node.m_leafStartIdx + node.m_leafCnt; i++) {
		if(m_leafs[i].m_leaf == val) {
			// found, remove it
			node.m_leafCnt--;
			m_leafs[i] = m_leafs[node.m_leafStartIdx + node.m_leafCnt];
			i--;
		}
	}
	// now update parents
	Node* curNode = node.m_parent;
	while(curNode != nullptr) {
		curNode->m_leafCnt--;
		for(eU32 i = 0; i < 2; i++) 
			if(curNode->m_children[i]) {
				if(m_nodeArray[curNode->m_children[i]].m_leafCnt == 0) 
					curNode->m_children[i] = 0;
			}
		curNode = curNode->m_parent;
	}
}

void eGenericKDTree::finishLoading() {
	m_leafsBuffer.reserve(m_leafs.size());
	m_nodeArray.clear();
	m_nodeArray.reserve(m_leafs.size());
	{
		ePROFILER_ZONE("KD Tree: Finish Loading");
		splitLeafs(nullptr, 0, 0, m_leafs.size());
	}
}


eU32 eGenericKDTree::findKAndSort(eU32 axis, eU32 start, eU32 len, eU32 rank) {
	eASSERT(len > 0);
	// copy all entries
	m_leafsBuffer.clear();
	for(eU32 i = start; i < start + len; i++) 
		m_leafsBuffer.append(m_leafs[i]);

	const eF32 pivot = m_leafsBuffer[eRandom(0, len, this->randomSeed)].m_position[axis];

	eU32 s = 0;
	eU32 b = 0;
	eU32 eqTrans = len >> 1;
	for(eU32 i = 0; i < len; i++) {
		eBool isSmaller = m_leafsBuffer[i].m_position[axis] < pivot;
		if(m_leafsBuffer[i].m_position[axis] == pivot)
			if(eqTrans > 0)
				eqTrans--;
			else
				isSmaller = eTRUE;
		if(isSmaller) {
			m_leafs[start + s] = m_leafsBuffer[i];
			s++;
		} else {
			b++;
			m_leafs[start + len - b] = m_leafsBuffer[i];
		}
	}
	if(rank == s) 
		return s;
	if(s < rank)
		return s + findKAndSort(axis, start + s, b, rank - s);
	return findKAndSort(axis, start, s, rank);
}


eU32 eGenericKDTree::splitLeafs(Node* parent, eU32 axis, eU32 startIdx, eU32 len) {
	eU32 nodeIdx = m_nodeArray.size();
	Node& node = m_nodeArray.append();
	node.m_parent = parent;

	node.m_leafStartIdx = startIdx;
	node.m_leafCnt = len;

	if(len <= KDTREE2_MAX_CHILDREN_PER_NODE) {
		node.m_children[0] = 0;
		node.m_children[1] = 0;
	} else {
		axis = (axis + 1) % 3;

		eU32 firstHalfLength = findKAndSort(axis, startIdx, len, len >> 1);
		node.m_splitVal = m_leafs[startIdx + firstHalfLength].m_position[axis];

		node.m_children[0] = splitLeafs(&node, axis, startIdx, firstHalfLength); 
		node.m_children[1] = splitLeafs(&node, axis, startIdx + firstHalfLength, len - firstHalfLength);
	}
	return nodeIdx;
}

void eGenericKDTree::getKNearest(eArray<Result>& results, eU32 k, const eVector3& position) {
	ePROFILER_FUNC();
	results.clear();
	results.resize(k + KDTREE2_MAX_CHILDREN_PER_NODE);
	// a small hack, set maximum element distance so that it is always defined
	eF32& maxElementDistanceSqr = (*((&results[0]) + k - 1)).distanceSqr;
	maxElementDistanceSqr = eF32_MAX;
	eU32 numRes = 0;
	getKNearest(m_nodeArray[0], results, numRes, 0, k, position, maxElementDistanceSqr);
	results.resize(numRes);
#ifdef eDEBUG
/*
	if(distFunc == nullptr) {
		const Entry* closest = nullptr;
		eF32 closestDist = eF32_MAX;
		for(eU32 i = 0; i < m_leafs.size(); i++) {
			eF32 dist = (m_leafs[i].m_aabb.getCenter() - position).sqrLength();
			if((closest == nullptr) ||
				(dist < closestDist)) {
					closest = &m_leafs[i];
					closestDist = dist;
			}
		}
		eASSERT(closest->m_leaf == results[0].t);
	}
*/
#endif
}

void eGenericKDTree::getKNearest(Node& node, eArray<Result>& results, eU32& numRes, eU32 axis, eU32 k, const eVector3& position, eF32& maxElementDistanceSqr) {
	if(node.m_children[0] || node.m_children[1]) {
		// tree node
		axis = (axis + 1) % 3;

		eU32 children[] = {node.m_children[0], node.m_children[1]};
		// determine order
		if(position[axis] >= node.m_splitVal)
			eSwap(children[0], children[1]);

		if(children[0])
			getKNearest(m_nodeArray[children[0]], results, numRes, axis, k, position, maxElementDistanceSqr);
		if(children[1] &&
			((numRes != k) || (eSqr(position[axis] - node.m_splitVal) < maxElementDistanceSqr)))
				getKNearest(m_nodeArray[children[1]], results, numRes, axis, k, position, maxElementDistanceSqr);
	} else {
		// leaf node
		for(eU32 i = node.m_leafStartIdx; i < node.m_leafStartIdx + node.m_leafCnt; i++) {
			const eF32 distanceSqr = (m_leafs[i].m_position - position).sqrLength();
			if(distanceSqr >= maxElementDistanceSqr)
				continue;
			
			// increase results size up to k
			numRes = eMin(k, numRes + 1);
			
			// move result elements and find insertion index
			eU32 rpos = numRes;
			while((rpos > 0) && (results[rpos - 1].distanceSqr > distanceSqr)) {
				results[rpos] = results[rpos - 1];
				rpos--;
			}
			
			// set result
			Result &r = results[rpos];
			r.distanceSqr = distanceSqr;
			r.t = m_leafs[i].m_leaf;
			r.node = &node;

		}
	}
}

