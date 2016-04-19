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

#ifdef ePLAYER
	extern eIOperator* globalCreateOp(eU32 type);
#endif

#ifdef eEDITOR

eOpMemoryMgr::eOpMemoryMgr()
{
    setBudget(eOC_MESH, 256*1024*1024); // 256 MB
    setBudget(eOC_BMP, 512*1024*1024);  // 512 MB
}

eOpMemoryMgr::~eOpMemoryMgr()
{
    for (eU32 i=0; i<m_budgets.size(); i++)
        eDelete(m_budgets[i]);
}

void eOpMemoryMgr::setBudget(eOpClass opc, eU32 budgetSize)
{
    eOpClassBudget *ocb = _findBudget(opc);
    if (!ocb)
        ocb = m_budgets.append(new eOpClassBudget);

    ocb->opClass = opc;
    ocb->maxResSize = budgetSize;
    ocb->usedResSize = 0;
    ocb->warned = eFALSE;
}

const eOpClassBudget * eOpMemoryMgr::getBudget(eOpClass opc) const
{
    return _findBudget(opc);
}

void eOpMemoryMgr::touch(eIOperator *op)
{
    eOpClassBudget *ocb = _findBudget(op->getMetaInfos().output);
    if (!ocb) // budget for operator class found?
        return;

    const eU32 newResSize = op->getResultSize();
    eU32 i;

    for (i=0; i<ocb->opsLru.size() && ocb->opsLru[i].op!=op; i++);

    if (i >= ocb->opsLru.size())
    {
        eOpBudgetSlice &obs = ocb->opsLru.append(eOpBudgetSlice());
        obs.op = op;
        obs.oldResSize = 0;
    }

    eOpBudgetSlice tmp = ocb->opsLru[i];

    for (eU32 j=i; j>0; j--)
        ocb->opsLru[j] = ocb->opsLru[j-1];

    ocb->opsLru[0] = tmp;

    eOpBudgetSlice &obs = ocb->opsLru.first();
    ocb->usedResSize -= obs.oldResSize;
    ocb->usedResSize += newResSize;
    obs.oldResSize = newResSize;
}

void eOpMemoryMgr::remove(eIOperator *op)
{
    for (eU32 i=0; i<m_budgets.size(); i++)
    {
        eOpClassBudget *budget = m_budgets[i];
        if (budget->opClass == op->getMetaInfos().output)
        {
            for (eU32 j=0; j<budget->opsLru.size(); j++)
            {
                const eOpBudgetSlice &obs = budget->opsLru[j];
                if (obs.op == op)
                {
                    budget->usedResSize -= obs.oldResSize;
                    budget->opsLru.removeAt(j);
                    return;
                }
            }
        }
    }
}

void eOpMemoryMgr::tidyUp()
{
    for (eU32 i=0; i<m_budgets.size(); i++)
    {
        eOpClassBudget *ocb = m_budgets[i];
            
        for (eInt j=(eInt)ocb->opsLru.size()-1; j>=0; j--)
        {
            if (ocb->usedResSize <= ocb->maxResSize)
                break;

            eIOperator *op = ocb->opsLru[j].op;
            if (!op->getBlocked())
            {
                ocb->usedResSize -= ocb->opsLru[j].oldResSize;
                op->freeResult();
                op->setChanged();
                ocb->opsLru.removeAt(j);
            }
        }

        if (ocb->usedResSize > ocb->maxResSize)
        {
            if (!ocb->warned)
            {
                eASSERT(!ocb->opsLru.isEmpty());
                const eString &category = ocb->opsLru[0].op->getResultMetaInfos().category;
                eWriteToLog(eString("Warning: Budget for operator category '")+category+"' too small!");
                ocb->warned = eTRUE;
            }
        }
        else
            ocb->warned = eFALSE;
    }
}

eOpClassBudget * eOpMemoryMgr::_findBudget(eOpClass opc) const
{
    for (eU32 i=0; i<m_budgets.size(); i++)
        if (m_budgets[i]->opClass == opc)
            return m_budgets[i];

    return nullptr;
}

eOpMemoryMgr eIOperator::m_memMgr;

#endif

eIOperator::eIOperator() :
#ifdef eEDITOR
    m_cycle(eFALSE),
    m_blocked(eFALSE),
    m_width(4),
    m_bypassed(eFALSE),
    m_hidden(eFALSE),
    m_error(eOE_OK),
    m_ownerPage(nullptr),
#else
    m_numVisits(0),
#endif
    m_changed(eTRUE),
    m_visited(eFALSE),
    m_visited2(eFALSE),
    m_subTime(0.0f)
{
    // generate a new random ID
    eRandomize(eTimer::getTimeMs());

    do
    {
        m_id = eRandom();
    }
    while (eDemoData::existsOperator(m_id));
}

eIOperator::~eIOperator()
{
    _clearParameters();

#ifdef eEDITOR
    m_memMgr.remove(this);
#endif
}

eOpProcessResult eIOperator::process(eF32 time, eOpCallback callback, ePtr param)
{
    ePROFILER_FUNC();
    eASSERT(time >= 0.0f);

    eIOpPtrArray stackOps;
    eOpProcessResult res = eOPR_NOCHANGES;
    eU32 opCount = 0;

    stackOps.reserve(eDemoData::getTotalOpCount());
    getOpsInStack(stackOps);

#ifdef ePLAYER
    // required in player as calls to process() can
    // be nested (player main calls demo operator's
    // process(), that one arrives at the linked loading
    // demo operator, calls the associated callback
    // which again calls the loading operator's
    // process() routine => problem!
    if (stackOps.isEmpty())
        return res;
#endif

#ifdef eEDITOR
    for (eU32 i=0; i<stackOps.size(); i++)
        stackOps[i]->m_blocked = eTRUE;
#endif

    if (callback && !callback(0, stackOps.size(), param))
        res = eOPR_CANCELED;

    _processIntern(time, callback, param, opCount, stackOps.size(), res);

    for (eU32 i=0; i<stackOps.size(); i++)
    {
        stackOps[i]->m_visited = eFALSE;
        stackOps[i]->m_changed = eFALSE;

#ifdef eEDITOR
        stackOps[i]->m_blocked = eFALSE;
#else
        stackOps[i]->m_numVisits = 0;
#endif
    }

    return res;
}

void eIOperator::_processIntern(eF32 time, eOpCallback callback, ePtr param, eU32 &opCount, eU32 opsTotal, eOpProcessResult &res)
{
#ifdef ePLAYER
    eASSERT((m_metaInfos->type == ePLAYER_OPTYPE_eDemoOp) || (m_numVisits < m_outputOps.size()));
#endif

	//if (m_id == 106 && m_numVisits > 0)
	//	_asm int 3;

    if (!m_visited)
    {
        // sequencer operators have a subtract time set
        // to map the absolute demo time to relative
        // sequencer entry times
        time -= m_subTime;

        // recursively process input opereators
        m_visited = eTRUE;
        for (eU32 i=0; i<m_inputOps.size(); i++)
        {
            m_inputOps[i]->_processIntern(time, callback, param, opCount, opsTotal, res);
            if (res == eOPR_CANCELED)
                return;
        }

        // update operator and reexecute if dirty
        _animateParameters(time);
        if (m_changed)
        {
            _preExecute();
            _callExecute();
            res = eOPR_CHANGES;
        }

        // update status information
        opCount++;
        if (callback && !callback(opCount, opsTotal, param))
            res = eOPR_CANCELED;

        // perform operator memory management
#ifdef eEDITOR
        m_memMgr.touch(this);
        m_memMgr.tidyUp();
#else
        // in player free all operators which won't be referenced
        // anymore and which are not part of an animated stack
        for (eU32 i=0; i<m_inputOps.size(); i++)
        {
            eIOperator *op = m_inputOps[i];
            op->m_numVisits++; // important: postpone increment to caller!
/*
			if (op->m_id == 106)
				__asm int 3;
*/
            // is any output animiated?
            eBool animed = eFALSE;
            for (eU32 j=0; j<op->m_outputOps.size(); j++)
                animed |= op->m_outputOps[j]->isAnimated();

            if (op->m_numVisits == op->m_outputOps.size() && !animed)
                op->freeResult(); // do not set to changed here (no reexecute wanted!)
        }
#endif
    }
}

eU32 eIOperator::getResultSize() const
{
    return 0;
}

void eIOperator::freeResult()
{
}

// does a binary search for operator with given ID.
// index of operator with given ID is returned,
// or -(insertat+1) if operator couldn't be found on
// this page. this value can easily be transformed into
// the position to insert the operator into the array.
eInt eIOperator::binarySearchOp(const eIOpPtrArray &ops, eID opId)
{
    eInt startIndex = 0;
    eInt stopIndex = ops.size()-1;

    while (startIndex <= stopIndex)
    {
        const eInt mid = (startIndex+stopIndex)/2; 
        eASSERT(mid >= 0 && mid < (eInt)ops.size());

        if (opId > ops[mid]->getId())
            startIndex = mid+1; // continue in top half
        else if (opId < ops[mid]->getId()) 
            stopIndex = mid-1; // continue in bottom half
        else
            return mid; // found it
    }

    // not found
    return -(startIndex+1);
}

eIOperator * eIOperator::newInstance(eU32 type)
{
#ifdef eEDITOR
    for (eU32 i=0; i<getAllMetaInfos().size(); i++)
        if (getAllMetaInfos()[i]->type == type)
            return getAllMetaInfos()[i]->createOp();

    return nullptr;
#else
	return globalCreateOp(type);
#endif
}

void eIOperator::setChanged(eBool reconnect, eBool force)
{
#ifdef eEDITOR
    if (reconnect)
        eDemoData::connectPages();
#endif

    // avoid traversing graph multiple times
    if (!m_changed || force)
    {
        for (eU32 i=0; i<m_outputOps.size(); i++)
            m_outputOps[i]->setChanged();

        m_changed = eTRUE;
    }
}

void eIOperator::setSubTime(eF32 subTime)
{
    eASSERT(subTime >= 0.0f);
    m_subTime = subTime;
}

eID eIOperator::getId() const
{
    return m_id;
}

eBool eIOperator::isAnimated() const
{
    ePROFILER_FUNC();

    if (!m_script.byteCode.isEmpty())
        return eTRUE;

    for (eU32 i=0; i<m_inputOps.size(); i++)
        if (m_inputOps[i]->isAnimated())
            return eTRUE;

    return eFALSE;
}

eU32 eIOperator::getParameterCount() const
{
    return m_params.size();
}

eU32 eIOperator::getInputOpCount() const
{
    return m_inputOps.size();
}

eU32 eIOperator::getOutputOpCount() const
{
    return m_outputOps.size();
}

eU32 eIOperator::getAboveOpCount() const
{
    return m_aboveOps.size();
}

eU32 eIOperator::getBelowOpCount() const
{
    return m_belowOps.size();
}

eU32 eIOperator::getLinkOutOpCount() const
{
    return m_linkOutOps.size();
}

eU32 eIOperator::getLinkInOpCount() const
{
    return m_linkInOps.size();
}

const eParameter & eIOperator::getParameter(eU32 index) const
{
    eASSERT(index < m_params.size());
    return *m_params[index];
}

eParameter & eIOperator::getParameter(eU32 index)
{
    eASSERT(index < m_params.size());
    return *m_params[index];
}

eIOperator * eIOperator::getInputOp(eU32 index) const
{
    eASSERT(index < m_inputOps.size());
    return m_inputOps[index];
}

eIOperator * eIOperator::getOutputOp(eU32 index) const
{
    eASSERT(index < m_outputOps.size());
    return m_outputOps[index];
}

eIOperator * eIOperator::getAboveOp(eU32 index) const
{
    eASSERT(index < m_aboveOps.size());
    return m_aboveOps[index];
}

eIOperator * eIOperator::getBelowOp(eU32 index) const
{
    eASSERT(index < m_belowOps.size());
    return m_belowOps[index];
}

eIOperator * eIOperator::getLinkOutOp(eU32 index) const
{
    eASSERT(index < m_linkOutOps.size());
    return m_linkOutOps[index];
}

eIOperator * eIOperator::getLinkInOp(eU32 index) const
{
    eASSERT(index < m_linkInOps.size());
    return m_linkInOps[index];
}

eBool eIOperator::getChanged() const
{
    return m_changed;
}


const eOpMetaInfos & eIOperator::getMetaInfos() const
{
    return *m_metaInfos;
}

// implemented as static function variable instead
// of static class attribute to avoid order of
// initialization issues with other static registerers
eOpMetaInfosArray & eIOperator::getAllMetaInfos()
{
    static eOpMetaInfosArray ami;
    return ami;
}

eIOperator * eIOperator::getResultOp()
{
    eIOperator *op = this;

    if (!m_visited2)
    {
        m_visited2 = eTRUE;

        if (m_metaInfos->output != eOC_ALL)
            op = this;
        else if (m_aboveOps.size())
            op = m_aboveOps[0]->getResultOp();
        else if (m_linkOutOps.size())
            op = m_linkOutOps[0]->getResultOp();

        m_visited2 = eFALSE;
    }

    return op;
}

eOpClass eIOperator::getResultClass() const
{
    return getResultMetaInfos().output;
}

const eOpMetaInfos & eIOperator::getResultMetaInfos() const
{
    // cast this to non-const as we now that
    // we won't change the result operator
    return ((eIOperator *)this)->getResultOp()->getMetaInfos();
}

eScript & eIOperator::getScript()
{
    return m_script;
}

// performs a depth first search on the operator DAG
void eIOperator::getOpsInStack(eIOpPtrArray &ops)
{
    _getOpsInStackVisit(ops);

    for (eU32 i=0; i<ops.size(); i++)
        ops[i]->m_visited = eFALSE;
}

#ifdef eEDITOR

eOpMemoryMgr & eIOperator::getMemoryMgr()
{
    return m_memMgr;
}

eOperatorPage * eIOperator::getOwnerPage() const
{
    return m_ownerPage;
}

// returns false if there was not
// interaction (operator wasn't changed)
eBool eIOperator::doEditorInteraction(eSceneData &sd, eOpInteractionInfos &oii)
{
    return eFALSE;
}

eString eIOperator::compileScript(const eString &source)
{
    // add operator parameters
	eArray<eU32> paramNrs;
    for (eU32 i=0; i<m_params.size(); i++)
		paramNrs.append(i);
	return compileScript(source, paramNrs);
}

eString eIOperator::compileScript(const eString &source, eArray<eU32>& usedParams)
{
    // add external variables
    eArray<eScriptExtVar> extVars;

    // time expected as first variable
    eScriptExtVar &ev = extVars.push();
    ev.constant = eTRUE;
    eStrCopy(ev.name, "time");
    ev.type = ST_SCALAR;

    // add operator parameters
    for (eU32 i=0; i<usedParams.size(); i++)
    {
        const eParameter &p = *m_params[usedParams[i]];
        if (p.isAnimatable())
        {
            eScriptExtVar &ev = extVars.push();
            ev.constant = eFALSE;
            eStrCopy(ev.name, p.getName());
            ev.type = (p.getComponentCount() == 1 ? ST_SCALAR : ST_VECTOR);

            // replace spaces by underscores so that
            // they can be accessed in scripts.
            for (eU32 j=0; j<eStrLength(ev.name); j++)
                if (!eIsAlphaNumeric(ev.name[j]))
                    ev.name[j] = '_';
        }
    }

    // compile script
	eArray<eU32> varUsages;
    eArray<eID> oldRefOps = m_script.refOps;
    eScriptCompiler sc;
    sc.compile(source, m_script, extVars, varUsages);

	m_scriptExtVarUsages.clear();
	// translate script var usages 
	eU32 uptr = 1; // skip time parameter
    for (eU32 i=0; i<usedParams.size(); i++)
		if(m_params[usedParams[i]]->isAnimatable()) {
			m_scriptExtVarUsages.append(varUsages[uptr]);
			uptr++;
		}
		else
			m_scriptExtVarUsages.append(0);

    // page has to be reconnected if referenced
    // operators have changed
    setChanged(oldRefOps != m_script.refOps);
    return sc.getErrors();
}

eU32	eIOperator::getScriptParamUsage(eU32 paramNr) const {
	return m_scriptExtVarUsages[paramNr]; 
}


void eIOperator::setUserName(const eString &userName)
{
    m_userName = userName;
}

// requires user call of page reconnect
void eIOperator::setBypassed(eBool bypass)
{
    m_bypassed = bypass;
}

// requires user call of page reconnect
void eIOperator::setHidden(eBool hidden)
{
    m_hidden = hidden;
}

eBool eIOperator::getBlocked() const
{
    return m_blocked;
}

const ePoint & eIOperator::getPosition() const
{
    return m_pos;
}

const eString & eIOperator::getUserName() const
{
    return m_userName;
}

eBool eIOperator::getBypassed() const
{
    return m_bypassed;
}

eBool eIOperator::getHidden() const
{
    return m_hidden;
}

eU32 eIOperator::getWidth() const
{
    return m_width;
}

eOpError eIOperator::getError() const
{
    return m_error;
}

eBool eIOperator::getStaticParamsAllowed() const {
	return m_allowStaticParameters;
}

#endif

void eIOperator::_preExecute()
{
}

void eIOperator::_initialize()
{
}

void eIOperator::_setupParams()
{
}

void eIOperator::_deinitialize()
{
}

#pragma warning(disable : 4731) // ebp changed warning

void eIOperator::_callExecute()
{
    // create array with stack data
    eU32 *stack = eALLOC_STACK(eU32, m_params.size());
    eU32 count = 0;

    for (eInt i=0; i<(eInt)m_params.size(); i++)
    {
        const eParamValue &val = m_params[i]->getAnimValue();

        switch (m_params[i]->getType())
        {
        case ePT_IXY:
        case ePT_IXYZ:
        case ePT_IXYXY:
        case ePT_FXY:
        case ePT_FXYZ:
        case ePT_FXYZW:
            stack[count++] = (eU32)&val.fxyz;
            break;

        case ePT_BOOL:
        case ePT_ENUM:
        case ePT_FLAGS:
        case ePT_INT:
            stack[count++] = *(eU32 *)&val.integer;
            break;

        case ePT_FLOAT:
            stack[count++] = *(eU32 *)&val.flt;
            break;

        case ePT_STR:
        case ePT_TEXT:
        case ePT_FILE:
            stack[count++] = (eU32)(const eChar *)val.string;
            break;

        case ePT_RGB:
        case ePT_RGBA:
            stack[count++] = (eU32)&val.color;
            break;

        case ePT_PATH:
            stack[count++] = (eU32)&val.path;
            break;

        case ePT_LINK:
            stack[count++] = (eU32)eDemoData::findOperator(val.linkedOpId);
            break;
#ifdef eEDITOR
        case ePT_LABEL:
            stack[count++] = 0; // add dummy value for a label
			break;
#endif
        }
    }

    // call execute handler
#ifdef eEDITOR
    const ePtr func = m_metaInfos->execFunc;
#else
    const ePtr func = globalOpExecFunction(m_metaInfos->type);
#endif
    const ePtr stackPtr = (count ? &stack[0] : nullptr);

    __asm
    {
        mov     eax, dword ptr [func]     // store stack variables before
        mov     ebx, dword ptr [this]     // the new stack frame is installed
        mov     ecx, dword ptr [count]    // (they won't be accessible anymore
        mov     esi, dword ptr [stackPtr] // afterwards)
        push    ebp
        mov     ebp, esp
        sub     esp, ecx // space for number of parameters * sizeof(eU32) (= 4)
        sub     esp, ecx
        sub     esp, ecx
        sub     esp, ecx
        mov     edi, esp // copy parameters on stack
        rep     movsd
        mov     ecx, ebx // this-call so provide this-pointer
        call    eax      // this-call requires no stack clean up
        mov     esp, ebp // remove stack frame
        pop     ebp
    }
}

#pragma warning(default : 4731) // ebp changed warning

void eIOperator::_animateParameters(eF32 time)
{
    ePROFILER_FUNC();
    eASSERT(time >= 0.0f);

    if (!m_params.size() || m_script.byteCode.isEmpty())
        return;

    // setup external variables (from parameters)
    eF32 *extVars = eALLOC_STACK(eF32, m_params.size()*4+1);
    eU32 index = 1;
    extVars[0] = time;

    for (eU32 i=0; i<m_params.size(); i++)
    {
        const eParameter &p = *m_params[i];
        if (!p.isAnimatable())
            continue;

        for (eInt j=0; j<(p.getComponentCount() == 1 ? 1 : 4); j++)
        {
            switch (p.getClass())
            {
            case ePC_FLT:
                extVars[index++] = eVector4(p.getAnimValue().fxyzw)[j];
                break;
            case ePC_INT:
                extVars[index++] = (eF32)eRect(p.getAnimValue().ixyxy)[j];
                break;
            case ePC_COL:
                extVars[index++] = (eF32)p.getAnimValue().color[j];
                break;
            }
        }
    }

    // execute animation script
    eScriptVm vm;
    vm.execute(m_script, extVars);

    // write back script variables to parameters
    for (eU32 i=0, index=1; i<m_params.size(); i++)
    {
        eParameter &p = *m_params[i];
        if (!p.isAnimatable())
            continue;

        const eParamValue oldAnimVal = p.getAnimValue();

        for (eInt j=0; j<(p.getComponentCount() == 1 ? 1 : 4); j++)
        {
            switch (p.getClass())
            {
            case ePC_FLT:
                ((eF32 *)&p.getAnimValue().fxyzw)[j] = eClamp(p.getMin(), extVars[index++], p.getMax());
                break;
            case ePC_INT:
                ((eInt *)&p.getAnimValue().ixyxy)[j] = eFtoL(eClamp(p.getMin(), extVars[index++], p.getMax()));
                break;
            case ePC_COL:
                p.getAnimValue().color[j] = eFtoL(eClamp(0.0f, extVars[index++], 255.0f));
                break;
            }
        }

        if (!eMemEqual(&oldAnimVal, &p.getAnimValue(), sizeof(eFXYZW)+sizeof(eColor)))
            setChanged();
    }
}

void eIOperator::_clearParameters()
{
    for (eU32 i=0; i<m_params.size(); i++)
        eDelete(m_params[i]);

    m_params.clear();
}

void eIOperator::_getOpsInStackVisit(eIOpPtrArray &ops)
{
    if (!m_visited)
    {
        for (eU32 i=0; i<m_inputOps.size(); i++)
            m_inputOps[i]->_getOpsInStackVisit(ops);

        ops.append(this);
        m_visited = eTRUE;
    }
}

#ifdef ePLAYER
void eIOperator::setupParameterDefinitions() {
	eS32 cnt = getMetaInfos().type;
	const eU8* cntPtr = demodata_parsCount;
	const eU8* parPtr = demodata_pars;
	const eF32* minMaxPtr = (const eF32*)demodata_parsMinMax;
	while(cnt >= 0) {
		eU32 parCnt = *(cntPtr++);
		while(parCnt > 0) {
			parCnt--;
			// more parameters
			const eParamType type = (eParamType)*(parPtr++);
			eF32 min = -eF32_MAX;
			eF32 max = eF32_MAX;
			if(*(parPtr++) != 0) {
				min = *(minMaxPtr++);
				max = *(minMaxPtr++);
			}
			if(cnt == 0) { // right op
				eOP_PARAM_ADD(type, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, min, max, DUMMY);
			}
		}
		cnt--;
	}
}
#endif
