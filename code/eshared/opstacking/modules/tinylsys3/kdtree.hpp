/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *   This file is part of
 *       _______   ______________  ______     _____
 *      / ____/ | / /  _/ ____/  |/  /   |   |__  /
 *     / __/ /  |/ // // / __/ /|_/ / /| |    /_ <
 *    / /___/ /|  // // /_/ / /  / / ___ |  ___/ /
 *   /_____/_/ |_/___/\____/_/  /_/_/  |_| /____/.
 *
 *   Copyright © 2003-2010 Brain Control, all rights reserved.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef GENERIC_KDTREE_HPP
#define GENERIC_KDTREE_HPP

#include "../../../eshared.hpp"

#define KDTREE2_MAX_CHILDREN_PER_NODE	64

class eGenericKDTree {
public:
	class Entry {
	public:
		eVector3	m_position;
		const void*	m_leaf;
	};

	typedef struct Node {
		eU32			m_children[2];
		Node*			m_parent;
		eU32			m_leafStartIdx;
		eU32			m_leafCnt;
		eF32			m_splitVal;
	};

	eU32 randomSeed;
	eGenericKDTree();
	void clear();
	void add(const eVector3& position, const void* value);
	void remove(const void* val, Node& node);
	void finishLoading();
	eU32 findKAndSort(eU32 axis, eU32 start, eU32 len, eU32 rank);
	eU32 splitLeafs(Node* parent, eU32 axis, eU32 start, eU32 len);

	typedef struct Result {
		eF32			distanceSqr;
		const void*		t;
		Node*			node;
	};

	void getKNearest(eArray<Result>& results, eU32 k, const eVector3& position);

private:

	void getKNearest(Node& node, eArray<Result>& results, eU32& numRes, eU32 axis, eU32 k, const eVector3& position, eF32& maxElementDistanceSqr);

	eArray<Node>	m_nodeArray;
	eArray<Entry>	m_leafs;
	eArray<Entry>	m_leafsBuffer;
};

#endif // GENERIC_KDTREE_HPP
