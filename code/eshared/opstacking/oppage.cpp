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

#include "../eshared.hpp"

#ifdef eEDITOR

eOperatorPage::eOperatorPage(eID pageId)
{
    if (pageId != eNOID)
    {
        eASSERT(!eDemoData::existsPage(pageId));
        m_id = pageId;
    }
    else
    {
        // generate random new ID
        eRandomize(eTimer::getTimeMs());

        do
        {
            m_id = eRandom();
        }
        while (eDemoData::existsPage(m_id));
    }
}

eOperatorPage::~eOperatorPage()
{
    for (eU32 i=0; i<m_ops.size(); i++)
        eDelete(m_ops[i]);

    m_ops.clear();
}

eIOperator * eOperatorPage::addOperator(eU32 opType, const ePoint &pos, eInt width, eID opId)
{
    eASSERT(pos.x >= 0 && pos.x < eOPPAGE_WIDTH);
    eASSERT(pos.y >= 0 && pos.y < eOPPAGE_HEIGHT);

    eIOperator *op = eIOperator::newInstance(opType);
    if (!op) // null if operator type cannot be found
        return nullptr;

    op->m_width = (width > 0 ? width : op->m_width);
    op->m_pos = pos;
    op->m_ownerPage = this;

    if (!isPageFreeAt(op->m_pos, op->m_width))
    {
        eDelete(op);
        return nullptr;
    }
    
    if (opId != eNOID)
    {
        eASSERT(eDemoData::existsOperator(opId) == eFALSE);
        op->m_id = opId;
    }

    const eInt insertAt = eIOperator::binarySearchOp(m_ops, op->m_id);
    eASSERT(insertAt < 0); // operator shouldn't exist
    m_ops.insert(-insertAt-1, op);
    return op;
}

void eOperatorPage::removeOperator(eID opId)
{
    const eInt index = eIOperator::binarySearchOp(m_ops, opId);
    if (index >= 0)
    {
        eDelete(m_ops[index]);
        m_ops.removeAt(index);
    }
}

void eOperatorPage::connect(eU32 pass)
{
    eASSERT(pass < 6);

    static eIOpPtrArray changedOps;

    if (pass == 0)
    {
        for (eU32 i=0; i<m_ops.size(); i++)
        {
            eIOperator *op = m_ops[i];
            op->m_linkInOps.clear();
            op->m_linkOutOps.clear();
            op->m_outputOps.clear();
            op->m_aboveOps.clear();
            op->m_belowOps.clear();
        }
    }
    else if (pass == 1)
    {
        // connect operators
        for (eU32 i=0; i<m_ops.size(); i++)
        {
            // find operators above and below
            eIOperator *op = m_ops[i];
            eIOpPtrArray oldInputOps = op->m_inputOps;
            op->m_inputOps.clear();

            for (eU32 j=0; j<m_ops.size(); j++)
            {
                eIOperator *curOp = m_ops[j];

                if (eClosedIntervalsOverlap(op->m_pos.x, op->m_pos.x+op->m_width-1, curOp->m_pos.x, curOp->m_pos.x+curOp->m_width-1))
                {
                    if (!curOp->m_hidden && curOp->m_pos.y+1 == op->m_pos.y)
                    {
                        // bypass operators
                        while (curOp && curOp->m_bypassed)
                            curOp = (curOp->getAboveOpCount() > 0 ? curOp->getAboveOp(0) : nullptr);

                        // add if valid after bypassing
                        if (curOp)
                            op->m_aboveOps.append(curOp);
                    }
                }
            }

            op->m_aboveOps.sort(_sortByPosX);

            // find link out operators
            for (eU32 j=0; j<op->m_params.size(); j++)
                if (op->m_params[j]->getType() == ePT_LINK)
                    _appendLinkedOp(op->m_linkOutOps, op->m_params[j]->getBaseValue().linkedOpId);

            for (eU32 j=0; j<op->m_script.refOps.size(); j++)
                _appendLinkedOp(op->m_linkOutOps, op->m_script.refOps[j]);

            // setup input operators
            op->m_inputOps.append(op->m_aboveOps);
            op->m_inputOps.append(op->m_linkOutOps);

            // have above operators changed?
            if (oldInputOps.size() != op->m_inputOps.size())
                changedOps.append(op);
            else
            {
                for (eU32 j=0; j<oldInputOps.size(); j++)
                    if (!op->m_inputOps.contains(oldInputOps[j]))
                        changedOps.append(op);
            }
        }
    }
    else if (pass == 2)
    {
        // collect below operators
        for (eU32 i=0; i<m_ops.size(); i++)
            for (eU32 j=0; j<m_ops[i]->m_aboveOps.size(); j++)
                m_ops[i]->m_aboveOps[j]->m_belowOps.append(m_ops[i]);

        // inputs = above + link out + ref. by script
        // outputs = below + link in
        for (eU32 i=0; i<m_ops.size(); i++)
        {
            eIOperator *op = m_ops[i];
            op->m_outputOps.append(op->m_belowOps);

            for (eU32 j=0; j<op->m_linkOutOps.size(); j++)
            {
                op->m_linkOutOps[j]->m_linkInOps.append(op);
                op->m_linkOutOps[j]->m_outputOps.append(op);
            }
        }
    }
    else if (pass == 3)
    {
        // update error state of operators
        for (eU32 i=0; i<m_ops.size(); i++)
        {
            _updateOpValid(m_ops[i]);

            // new run for cycle detection
            for (eU32 j=0; j<m_ops.size(); j++)
                m_ops[j]->m_cycle = eFALSE;
        }
    }
    else if (pass == 4)
    {
        // reset flags
        for (eU32 j=0; j<m_ops.size(); j++)
        {
            m_ops[j]->m_visited = eFALSE;
            m_ops[j]->m_cycle = eFALSE;
        }
    }
    else if (pass == 5)
    {
        // flag changed operators
        for (eU32 i=0; i<changedOps.size(); i++)
            changedOps[i]->setChanged();

        changedOps.clear();
    }
}

void eOperatorPage::_appendLinkedOp(eIOpPtrArray &ops, eID opId) const
{
    eIOperator *linkedOp = eDemoData::findOperator(opId);
    if (linkedOp && !linkedOp->m_hidden)
        ops.append(linkedOp);
}

eBool eOperatorPage::resizeOperator(eIOperator *op, eU32 newWidth)
{
    eASSERT(newWidth > 0 && newWidth <= eOPPAGE_WIDTH);
    eASSERT(op->m_ownerPage == this);

    eIOpPtrArray ignoreOps;
    ignoreOps.append(op);

    if (!isPageFreeAt(op->m_pos, op->m_width, ignoreOps) || newWidth == op->m_width)
        return eFALSE;

    op->m_width = newWidth;
    return eTRUE;
}

eBool eOperatorPage::moveOperator(eIOperator *op, const ePoint &newPos)
{
    eIOpPtrArray ops;
    ops.append(op);
    return moveOperators(ops, op->m_pos-newPos);
}

eBool eOperatorPage::moveOperators(const eIOpPtrArray &ops, const ePoint &moveDist)
{
    if (areOpsMovable(ops, moveDist) && moveDist != ePoint(0, 0))
    {
        for (eU32 i=0; i<ops.size(); i++)
        {
            eASSERT(ops[i]->m_ownerPage == this);
            ops[i]->m_pos += moveDist;
        }

        return eTRUE;
    }

    return eFALSE;
}

eBool eOperatorPage::isPageFreeAt(const ePoint &pos, eU32 width, const eIOpPtrArray &ignoreOps) const
{
    for (eU32 i=0; i<m_ops.size(); i++)
    {
        const ePoint &opPos = m_ops[i]->m_pos;

        if (opPos.y == pos.y)
            if (eClosedIntervalsOverlap(opPos.x, opPos.x+m_ops[i]->m_width-1, pos.x, pos.x+width-1))
                if (!ignoreOps.contains(m_ops[i]))
                    return eFALSE;
    }

    return eTRUE;
}

eBool eOperatorPage::areOpsMovable(const eIOpPtrArray &ops, const ePoint &moveDist) const
{
    for (eU32 i=0; i<ops.size(); i++)
    {
        const eIOperator *op = ops[i];
        eASSERT(op->m_ownerPage == this);
        const ePoint &newPos = op->m_pos+moveDist;

        if (newPos.x < 0 || newPos.y < 0 || newPos.x+op->m_width > eOPPAGE_WIDTH || newPos.y >= eOPPAGE_HEIGHT)
            return eFALSE;
        else if (!isPageFreeAt(newPos, op->m_width, ops))
            return eFALSE;
    }

    return eTRUE;
}

void eOperatorPage::setUserName(const eString &userName)
{
    m_userName = userName;
}

const eString & eOperatorPage::getUserName() const
{
    return m_userName;
}

eID eOperatorPage::getId() const
{
    return m_id;
}

eU32 eOperatorPage::getOperatorCount() const
{
    return m_ops.size();
}

eIOperator * eOperatorPage::getOperatorById(eID opId) const
{
    const eInt index = eIOperator::binarySearchOp(m_ops, opId);
    return (index >= 0 ? m_ops[index] : nullptr);
}

eIOperator * eOperatorPage::getOperatorByPos(const ePoint &pos) const
{
    for (eU32 i=0; i<m_ops.size(); i++)
    {
        const ePoint &opPos = m_ops[i]->m_pos;

        if (opPos.y == pos.y)
            if (eClosedIntervalsOverlap(opPos.x, opPos.x+m_ops[i]->m_width-1, pos.x, pos.x))
                return m_ops[i];
    }

    return nullptr;
}

eIOperator * eOperatorPage::getOperatorByIndex(eU32 index) const
{
    eASSERT(index < m_ops.size());
    return m_ops[index];
}

void eOperatorPage::_updateOpValid(eIOperator *op) const
{
    // cycle found in stack?
    if (op->m_cycle)
    {
        op->m_error = eOE_STACK_CYCLIC;
        return;
    }

    op->m_error = eOE_OK;
    op->m_visited = eTRUE;
    op->m_cycle = eTRUE;

    // check for missing links
    for (eU32 i=0; i<op->m_params.size(); i++)
    {
        if (op->m_params[i]->isRequiredLink())
        {
            const eIOperator *linkedOp = eDemoData::findOperator(op->m_params[i]->getBaseValue().linkedOpId);
            if (!linkedOp || linkedOp->m_hidden)
                op->m_error = eOE_LINK_MISSING;
        }
    }

    // number of above operators in range?
    if (op->m_aboveOps.size() < op->m_metaInfos->minAbove)
        op->m_error = eOE_ABOVE_MISSING;
    else if (op->m_aboveOps.size() > op->m_metaInfos->maxAbove)
        op->m_error = eOE_ABOVE_TOOMANY;
    else
    {
        // all input operators valid?
        for (eU32 i=0; i<op->m_inputOps.size(); i++)
            if (op->m_inputOps[i]->getError() != eOE_OK)
                op->m_error = eOE_INPUT_ERRONEOUS;

        // all above operators of allowed type?
        for (eU32 i=0; i<op->m_aboveOps.size(); i++)
            if (!(op->m_metaInfos->above[i]&op->m_aboveOps[i]->getResultClass()))
                op->m_error = eOE_ABOVE_NOTALLOWED;
    }

    // update all outputs
    for (eU32 i=0; i<op->m_outputOps.size(); i++)
        _updateOpValid(op->m_outputOps[i]);

    op->m_cycle = eFALSE;
}

eBool eOperatorPage::_sortByPosX(eIOperator * const &op0, eIOperator * const &op1)
{
    return (op0->m_pos.x > op1->m_pos.x);
}

#endif