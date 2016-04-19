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

#ifndef DEMO_DATA_HPP
#define DEMO_DATA_HPP

enum eOpSerializationFlags
{
    eOPSF_ALREADY_STORED    = 0,
    eOPSF_DUPLICATE         = 1,
    eOPSF_ABOVE_END         = 2,
    eOPSF_LINKED_END        = 3,
    eOPSF_COUNT             = 4
};

#ifdef eEDITOR

struct eSHADER_ENTRY {
	eString name;
	eString shaderContent;
};

class eDemoData
{
private:
    struct OpExpReport
    {
        const eChar *       opCategory;
        const eChar *       opName;
        eU32                opType;
        eU32                numStoredOps;
        eU32                numUniqueOps;
        eU32                numDupOps;
        eU32                size;
        eF32                ratio;
    };

    typedef eArray<OpExpReport> OpExpReportArray;

public:
    static void             exportScript(eDemoScript &ds, eIDemoOp *demoOp);
    static void             free();
    static void             connectPages();
    static void             compileScripts();
    
    static eBool            existsOperator(eID opId);
    static eIOperator *     findOperator(eID opId);
    static eU32             getTotalOpCount();

    static eOperatorPage *  addPage(eID pageId=eNOID);
    static eBool            removePage(eID pageId);
    static void             clearPages();
    static eBool            existsPage(eID pageId);
    static eU32             getPageCount();
    static eOperatorPage *  getPageByIndex(eU32 index);
    static eOperatorPage *  getPageById(eID pageId);

	static eArray<eSHADER_ENTRY>	m_usedShaders;
	static eArray<eSHADER_ENTRY>	m_unusedShaders;

    static eArray<eU32>     m_new2OldOpTypes;
	static eArray<eString>	m_operatorSourceFiles;
private:
    static void             _writeOperator(eDemoScript &ds, eIOperator *op);
    static void             _writeCollectedOperators(eDemoScript &ds);
    static eU32             _writeParameter(eDemoScript &ds, eParameter &p);
    static eU32             _writePath(eDemoScript &ds, const ePath4 &path, eU32 component);
    static eIOperator *     _findOpDuplicate(eIOperator *op);
	static eIOperator *     _translateEquivalentOp(eIOperator *op);
	static void				_buildEquivalencyList(eIOperator *op);
	static void				_analyzeOpExport(eDemoScript &ds);
	static eIOperator*		_getStaticParam(eDemoScript &ds, eU32 opType, eU32 paramNr);
    static eID              _oldToNewOpId(eID opId);
    static eU8              _oldToNewOpType(eU32 opType);
    static OpExpReport &    _getOpReport(eIOperator *op);
    static eBool            _sortOpRepsByRatio(const OpExpReport &oer0, const OpExpReport &oer1);
	static void				_createListOfUsedShaders(eArray<eIOperator*>& usedOps);
	static void				_collectUsedOps(eIOperator* op, eArray<eIOperator*>& ops);
	static void				_writeShaders(eDemoScript &ds);

private:
    static eOpPagePtrArray  m_pages;
    static OpExpReportArray m_opReps;
    static eIOpPtrArray     m_storedOps;
	static eIOpPtrArray     m_equivalentOps;
    static eArray<eID>      m_new2OldOpIds;
	static eArray<const ePath4*>	m_pathesToStore;
};

#else

class eDemoData
{
public:
    static eIDemoOp *       importScript(eDemoScript &ds);

    static void             free();
    static eBool            existsOperator(eID opId);
    static eIOperator *     findOperator(eID opId);
    static eU32             getTotalOpCount();

	static	const eU8		demoData[];
	static void				readShaders(eDemoScript &ds);

private:
    static eIOperator *     _readOperator(eDemoScript &ds, eU8 cmd, eID &opId);
    static void             _readParameter(eDemoScript &ds, eParameter &p);
    static void             _readPath(eDemoScript &ds, ePath4 &path, eU32 component);
    static void				_readCollectedOps(eDemoScript &ds);

private:
    static eIOpPtrArray     m_ops;
	static eArray<ePath4*>	m_pathesToRead;
};

#endif

#endif // DEMO_DATA_HPP