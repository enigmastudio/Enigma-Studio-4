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

#ifndef IOPERATOR_HPP
#define IOPERATOR_HPP

class eIOperator;
class eIBitmapOp;
struct eOpMetaInfos;

typedef eArray<eIOperator *> eIOpPtrArray;
typedef eArray<const eIOperator *> eIOpConstPtrArray;
typedef eArray<const eOpMetaInfos *> eOpMetaInfosArray;
typedef eBool (* eOpCallback)(eU32 processed, eU32 total, ePtr param);

enum eOpClass
{
    eOC_BMP   = 1,
    eOC_MESH  = 2,
    eOC_MODEL = 4,
    eOC_FX    = 8,
    eOC_MISC  = 16,
    eOC_SEQ   = 32,
    eOC_DEMO  = 64,
    eOC_PATH  = 128,
    eOC_MAT   = 256,
    eOC_SONG  = 512,
    eOC_R2T   = 1024,
    eOC_SCENE = 2048,
    eOC_POV   = 4096,
    eOC_ALL   = eOC_BMP|eOC_MESH|eOC_MODEL|eOC_FX|eOC_MISC|eOC_SEQ|eOC_DEMO|eOC_PATH|eOC_MAT|eOC_SONG|eOC_R2T|eOC_SCENE|eOC_POV,
    eOC_COUNT = 13
};

enum eOpProcessResult
{
    eOPR_CANCELED,
    eOPR_CHANGES,
    eOPR_NOCHANGES
};

enum eOpError
{
    eOE_OK,
    eOE_ABOVE_TOOMANY,
    eOE_ABOVE_MISSING,
    eOE_ABOVE_NOTALLOWED,
    eOE_INPUT_ERRONEOUS,
    eOE_STACK_CYCLIC,
    eOE_LINK_MISSING,
};

struct eOpResult
{
};

struct eOpMetaInfos
{
#ifdef eEDITOR
	eString						className;
    eString                     name;
    eString                     category;
    eColor                      color;
    eChar                       shortcut;
    eU32                        minAbove;
    eU32                        maxAbove;
    eInt                        above[64];
#endif
    eOpClass                    output;
    eU32                        type;
#ifdef eEDITOR
    ePtr                        execFunc;
    eIOperator *                (*createOp)();
#endif
};

struct eOpInteractionInfos
{
    eOpInteractionInfos() :
        infoInst(infoMesh)
    {
    }

    eCamera                     cam;
    eUserInput                  input;
    eMesh                       infoMesh;
    eMeshInst                   infoInst;
    eTransform                  infoTransf;
    eSrtWidgetController        srtCtrl;
};

#ifdef eEDITOR

struct eOpBudgetSlice
{
    eIOperator *                op;
    eU32                        oldResSize;
};

struct eOpClassBudget
{
    eOpClass                    opClass;
    eU32                        usedResSize;
    eU32                        maxResSize;
    eArray<eOpBudgetSlice>      opsLru;
    eBool                       warned; // to verify out-of-memory warning is only shown once
};

class eOpMemoryMgr
{
public:
    eOpMemoryMgr();
    ~eOpMemoryMgr();

    void                        setBudget(eOpClass opc, eU32 budgetSize);
    const eOpClassBudget *      getBudget(eOpClass opc) const;

    void                        touch(eIOperator *op);
    void                        remove(eIOperator *op);
    void                        tidyUp();

private:
    eOpClassBudget *            _findBudget(eOpClass opc) const;

private:
    eArray<eOpClassBudget *>    m_budgets;
};

#endif

class eIOperator
{
    friend class eOperatorPage;
    friend class eDemoData;

public:
    eIOperator();
    virtual ~eIOperator();

    virtual eOpProcessResult    process(eF32 time, eOpCallback callback=nullptr, ePtr param=nullptr);
    virtual const eOpResult &   getResult() const = 0;
    virtual eU32                getResultSize() const;
    virtual void                freeResult();

    static eInt                 binarySearchOp(const eIOpPtrArray &ops, eID opId);
    static eIOperator *         newInstance(eU32 type);
    void                        setChanged(eBool reconnect=eFALSE, eBool force=eFALSE);
    void                        setSubTime(eF32 subTime);

    eID                         getId() const;
    eBool                       isAnimated() const;
    eU32                        getParameterCount() const;
    eU32                        getInputOpCount() const;
    eU32                        getOutputOpCount() const;
    eU32                        getAboveOpCount() const;
    eU32                        getBelowOpCount() const;
    eU32                        getLinkOutOpCount() const;
    eU32                        getLinkInOpCount() const;
    eParameter &                getParameter(eU32 index);
    const eParameter &          getParameter(eU32 index) const;
    eIOperator *                getInputOp(eU32 index) const;
    eIOperator *                getOutputOp(eU32 index) const;
    eIOperator *                getAboveOp(eU32 index) const;
    eIOperator *                getBelowOp(eU32 index) const;
    eIOperator *                getLinkOutOp(eU32 index) const;
    eIOperator *                getLinkInOp(eU32 index) const;
    eBool                       getChanged() const;
    eIOperator *                getResultOp();
    eOpClass                    getResultClass() const;
    const eOpMetaInfos &        getResultMetaInfos() const;
    eScript &                   getScript();
    void                        getOpsInStack(eIOpPtrArray &ops);
    const eOpMetaInfos &        getMetaInfos() const;
    static eOpMetaInfosArray &  getAllMetaInfos();

#ifdef eEDITOR
public:
    static eOpMemoryMgr &       getMemoryMgr();

    eOperatorPage *             getOwnerPage() const;
    virtual eBool               doEditorInteraction(eSceneData &sd, eOpInteractionInfos &oii);
    eString                     compileScript(const eString &source);
    eString                     compileScript(const eString &source, eArray<eU32>& usedParams);

    void                        setUserName(const eString &userName);
    void                        setBypassed(eBool bypass);
    void                        setHidden(eBool hidden);

    eBool                       getBlocked() const;
    const ePoint &              getPosition() const;
    const eString &             getUserName() const;
    eBool                       getBypassed() const;
    eBool                       getHidden() const;
    eU32                        getWidth() const;
    eOpError                    getError() const;

	eBool						getStaticParamsAllowed() const;
	eU32						getScriptParamUsage(eU32 paramNr) const;
#else
	void						setupParameterDefinitions();
	const	static eU8			demodata_parsCount[];
	const	static eU8			demodata_pars[];
	const	static eU8			demodata_parsMinMax[];
#endif

protected:
    virtual void                _preExecute();
    void                        _initialize();
    void                        _setupParams();
    void                        _deinitialize();
    void                        _callExecute();

private:
    void                        _processIntern(eF32 time, eOpCallback callback, ePtr param, eU32 &opCount, eU32 opsTotal, eOpProcessResult &res);
    void                        _animateParameters(eF32 time);
    void                        _clearParameters();
    void                        _getOpsInStackVisit(eIOpPtrArray &ops);

protected:
    eID                         m_id;
    eArray<eParameter *>        m_params;       // pointers that parameters are copyable (needed)
    const eOpMetaInfos *        m_metaInfos;
    eScript                     m_script;
    eBool                       m_changed;
    mutable eBool               m_visited;      // for graph traversal jobs
    mutable eBool               m_visited2;     // for nested graph traversals
    eF32                        m_subTime;      // for demo playback to calculate relative time
    eIOpPtrArray                m_aboveOps;     // operators above this one
    eIOpPtrArray                m_belowOps;     // operators below this one
    eIOpPtrArray                m_linkInOps;    // operators linking this one
    eIOpPtrArray                m_linkOutOps;   // operators linked by this one
    eIOpPtrArray                m_inputOps;     // = above + link out
    eIOpPtrArray                m_outputOps;    // = below + link in

#ifdef eEDITOR
    eOperatorPage *             m_ownerPage;
    eBool                       m_cycle;        // for cycle detection in DAG
    ePoint                      m_pos;
    eU32                        m_width;
    eBool                       m_bypassed;
    eBool                       m_hidden;
    eString                     m_userName;
    eOpError                    m_error;
    eBool                       m_blocked;      // freeable by memory manager?
	eBool						m_allowStaticParameters;
	eArray<eU32>				m_scriptExtVarUsages;
    static eOpMemoryMgr         m_memMgr;
#else
    eU32                        m_numVisits;
#endif
};

#endif // IOPERATOR_HPP