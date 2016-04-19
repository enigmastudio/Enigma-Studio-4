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

#ifndef OP_PAGE_HPP
#define OP_PAGE_HPP

#ifdef eEDITOR

enum eOpPageDimensions
{
    eOPPAGE_WIDTH = 65*3-1,
    eOPPAGE_HEIGHT = 95,
};

class eOperatorPage
{
    friend class eIOperator;

public:
    eOperatorPage(eID pageId=eNOID);
    ~eOperatorPage();

    eIOperator *        addOperator(eU32 opType, const ePoint &pos, eInt width=-1, eID opId=eNOID);
    void                removeOperator(eID opId);
    void                connect(eU32 pass); // pass in {0,...,5}
    
    eBool               resizeOperator(eIOperator *op, eU32 newWidth);
    eBool               moveOperator(eIOperator *op, const ePoint &newPos);
    eBool               moveOperators(const eIOpPtrArray &ops, const ePoint &moveDist);
    eBool               isPageFreeAt(const ePoint &pos, eU32 width, const eIOpPtrArray &ignoreOps=eIOpPtrArray()) const;
    eBool               areOpsMovable(const eIOpPtrArray &ops, const ePoint &moveDist) const;

    void                setUserName(const eString &userName);
    const eString &     getUserName() const;

    eID                 getId() const;
    eU32                getOperatorCount() const;
    eIOperator *        getOperatorById(eID opId) const;
    eIOperator *        getOperatorByPos(const ePoint &pos) const;
    eIOperator *        getOperatorByIndex(eU32 index) const;

private:
    void                _updateOpValid(eIOperator *op) const;
    void                _appendLinkedOp(eIOpPtrArray &ops, eID opId) const;
    static eBool        _sortByPosX(eIOperator * const &op0, eIOperator * const &op1);

private:
    eID                 m_id;
    eIOpPtrArray        m_ops;
    eString             m_userName;
};

typedef eArray<eOperatorPage *> eOpPagePtrArray;

#endif

#endif // OP_PAGE_HPP