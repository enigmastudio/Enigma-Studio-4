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

eOpPagePtrArray             eDemoData::m_pages;
eDemoData::OpExpReportArray eDemoData::m_opReps;
eIOpPtrArray                eDemoData::m_storedOps;
eIOpPtrArray                eDemoData::m_equivalentOps;
eArray<eID>                 eDemoData::m_new2OldOpIds;
eArray<eU32>                eDemoData::m_new2OldOpTypes;
eArray<eSHADER_ENTRY>		eDemoData::m_usedShaders;
eArray<eSHADER_ENTRY>		eDemoData::m_unusedShaders;
eArray<const ePath4*>		eDemoData::m_pathesToStore;
eArray<eString>				eDemoData::m_operatorSourceFiles;

// idea of script export/import:
//
// the script should be stored in such a way that it can be
// compressed as well as possible. therefore, data is never stored
// bit-wise (produces mostly random instead of redundant data) and
// the following optimizations are applied in order to reduce the
// initial size of data that has to be compressed by the exe-packer.
//
// optimizations: 
// - operator IDs are renumbered. that way they do not have to be
//   stored but can be recreated when importing the script.
// - first a block of all operator types is stored, second a block
//   of all parameters. that way the compressability of the script
//   is increased by improving data locality.
// - parameters are only stored if they differ from their default value.
// - duplicate operators are only stored once.

void eDemoData::exportScript(eDemoScript &ds, eIDemoOp *demoOp)
{
	// write shaders
	eArray<eIOperator*> opsInTree;
	_collectUsedOps(demoOp, opsInTree);
	_createListOfUsedShaders(opsInTree);
	_writeShaders(ds);

    // collect and store new operator type IDs
    eIOpPtrArray stackOps;
    demoOp->getOpsInStack(stackOps);
    m_new2OldOpTypes.clear();

    for (eU32 i=0; i<stackOps.size(); i++)
    {
        const eU32 opType = stackOps[i]->getResultOp()->getMetaInfos().type;
        if (!m_new2OldOpTypes.contains(opType))
            m_new2OldOpTypes.append(opType);
    }

    // export script and collect report
	m_pathesToStore.clear();
    m_opReps.clear();
    m_storedOps.clear();
	m_equivalentOps.clear();
	_buildEquivalencyList(demoOp);
    m_new2OldOpIds.clear();
    _writeOperator(ds, demoOp);
	_analyzeOpExport(ds);
	_writeCollectedOperators(ds);

	ds.calculateScriptStreamOrder();

    // pack script for size estimation
    eArithmeticPacker packer;
    eByteArray packed;
    packer.pack(ds.getFinalScript(), packed);

    // combine per operator statistics
    eU32 sumStoredOps = 0;
    eU32 sumUniqueOps = 0;
    eU32 sumDupOps = 0;
    eU32 sumSize = 0;
    eF32 sumRatio = 0.0f;

    for (eU32 i=0; i<m_opReps.size(); i++)
        sumSize += m_opReps[i].size;

    for (eU32 i=0; i<m_opReps.size(); i++)
    {
        OpExpReport &oer = m_opReps[i];
        oer.ratio = (eF32)oer.size/(eF32)sumSize;
        sumRatio += oer.ratio;
        sumDupOps += oer.numDupOps;
        sumStoredOps += oer.numStoredOps;
        sumUniqueOps += oer.numUniqueOps;
    }

    eASSERT(eAreFloatsEqual(sumRatio, 1.0f));
    m_opReps.sort(_sortOpRepsByRatio);

    // write export report to log
    eString buf;
    buf += "-------------------------------------------------------------------------------\n";
    buf += "    Export report\n";
    buf += "-------------------------------------------------------------------------------\n";
    buf += "Category\tName\tStored\tUnique\tDuplicates\tSize\tRatio\n";
    buf += "-------------------------------------------------------------------------------\n";

    for (eU32 i=0; i<m_opReps.size(); i++)
    {
        const OpExpReport &oer = m_opReps[i];
        buf += oer.opCategory;
        buf += "\t";
        buf += oer.opName;
        buf += "\t";
        buf += eIntToStr(oer.numStoredOps);
        buf += "\t";
        buf += eIntToStr(oer.numUniqueOps);
        buf += "\t";
        buf += eIntToStr(oer.numDupOps);
        buf += "\t";
        buf += eIntToStr(oer.size);
        buf += "\t";
        buf += eFloatToStr(oer.ratio*100.0f);
        buf += " %\n";
    }

    buf += "-------------------------------------------------------------------------------\n";
    buf += "\t\t";
    buf += eIntToStr(sumStoredOps);
    buf += "\t";
    buf += eIntToStr(sumUniqueOps);
    buf += "\t";
    buf += eIntToStr(sumDupOps);
    buf += "\t";
    buf += eIntToStr(sumSize);
    buf += "\t";
    buf += eFloatToStr(sumRatio*100.0f);
    buf += " %\n";
    buf += "-------------------------------------------------------------------------------\n";
    buf += "      Total operators:\t";
    buf += eIntToStr(stackOps.size());
    buf += "\n   Resolved operators:\t";
    buf += eIntToStr(stackOps.size()-sumStoredOps);
    buf += "\n    Final script size:\t";
    buf += eIntToStr(ds.getFinalScript().size());
    buf += "\n   Packed script size:\t";
    buf += eIntToStr(packed.size());
    buf += "\nPacked/unpacked ratio:\t";
    buf += eFloatToStr((eF32)packed.size()/(eF32)ds.getFinalScript().size());
    buf += "\n";

    eWriteToLog(buf);


}

void eDemoData::free()
{
    clearPages();
}

void eDemoData::connectPages()
{
    for (eU32 i=0; i<6; i++) // there are 6 connection passes
        for (eU32 j=0; j<m_pages.size(); j++)
            m_pages[j]->connect(i);
}

void eDemoData::compileScripts()
{
    for (eU32 i=0; i<m_pages.size(); i++)
    {
        for (eU32 j=0; j<m_pages[i]->getOperatorCount(); j++)
        {
            eIOperator *op = m_pages[i]->getOperatorByIndex(j);
            op->compileScript(op->getScript().source);
        }
    }
}

eBool eDemoData::existsOperator(eID opId)
{
    return (findOperator(opId) != nullptr);
}

eIOperator * eDemoData::findOperator(eID opId)
{
    for (eU32 i=0; i<m_pages.size(); i++)
    {
        eIOperator *op = m_pages[i]->getOperatorById(opId);
        if (op)
            return op;
    }

    return nullptr;
}

eU32 eDemoData::getTotalOpCount()
{
    eU32 count = 0;
    for (eU32 i=0; i<m_pages.size(); i++)
        count += m_pages[i]->getOperatorCount();

    return count;
}

eOperatorPage * eDemoData::addPage(eID pageId)
{
    return m_pages.append(new eOperatorPage(pageId));
}

eBool eDemoData::removePage(eID pageId)
{
    for (eU32 i=0; i<m_pages.size(); i++)
    {
        if (m_pages[i]->getId() == pageId)
        {
            eDelete(m_pages[i]);
            m_pages.removeAt(i);
            return eTRUE;
        }
    }

    return eFALSE;
}

void eDemoData::clearPages()
{
    for (eU32 i=0; i<m_pages.size(); i++)
        eDelete(m_pages[i]);

    m_pages.clear();
}

eBool eDemoData::existsPage(eID pageId)
{
    return (getPageById(pageId) != nullptr);
}

eU32 eDemoData::getPageCount()
{
    return m_pages.size();
}

eOperatorPage * eDemoData::getPageByIndex(eU32 index)
{
    eASSERT(index < m_pages.size());
    return m_pages[index];
}

eOperatorPage * eDemoData::getPageById(eID pageId)
{
    for (eU32 i=0; i<m_pages.size(); i++)
        if (m_pages[i]->getId() == pageId)
            return m_pages[i];

    return nullptr;
}

void eDemoData::_collectUsedOps(eIOperator* op, eArray<eIOperator*>& ops) {
    op = op->getResultOp(); // skip nop, load, store
	for(eU32 i = 0; i < ops.size(); i++)
		if(ops[i] == op)
			return;
	ops.append(op);
    for (eU32 i=0; i<op->getAboveOpCount(); i++)
		_collectUsedOps(op->getAboveOp(i), ops);

	// store linked in operators
    for (eU32 i=0; i<op->getLinkOutOpCount(); i++)
		_collectUsedOps(op->getLinkOutOp(i), ops);
}

void eDemoData::_writeOperator(eDemoScript &ds, eIOperator *op)
{
    op = op->getResultOp(); // skip nop, load, store
	op = _translateEquivalentOp(op);
    const eU8 newOpType = _oldToNewOpType(op->getMetaInfos().type);
    const eID newOpId = _oldToNewOpId(op->getId());
    OpExpReport &oer = _getOpReport(op);
    eU32 size = 0;

    // was this operator already stored before?
    // (in DAG multiple operators can link the same)
    if (m_storedOps.contains(op))
    {
        size += ds.writeU8(eOPSF_ALREADY_STORED);
        size += ds.writeU16(newOpId);
        return;
    }

    size += ds.writeU8(newOpType);
    oer.numUniqueOps++;
    ds.addUsedOp(op);

    m_storedOps.append(op);
    oer.numStoredOps++;

    // store input operators
    for (eU32 i=0; i<op->getAboveOpCount(); i++)
        _writeOperator(ds, op->getAboveOp(i));
    size += ds.writeU8(eOPSF_ABOVE_END);

	// store linked in operators
    for (eU32 i=0; i<op->getLinkOutOpCount(); i++)
        _writeOperator(ds, op->getLinkOutOp(i));
    size += ds.writeU8(eOPSF_LINKED_END);

    // retrieve report again cause recursive nature of this
    // function might have resize operator report list
    // and thus the reference obtained above is invalid
    _getOpReport(op).size += size;
}

void eDemoData::_writeCollectedOperators(eDemoScript &ds) {
	// write all op types in blocks, and within each block group the parameters
	eU32 numStatic = 0;
    for (eU32 i=0; i<m_new2OldOpTypes.size(); i++) {
		eU32 opType = m_new2OldOpTypes[i];
		eBool writeParams = eTRUE;
		eU32 paramNr = 0;
		eBool opsFound = eFALSE;
		while(writeParams) {
			eU32 paramsWritten = 0;
			eIOperator * firstOp = nullptr;
			for(eU32 o = 0; o < m_storedOps.size(); o++) {
				eIOperator * op = m_storedOps[o];
				if(opType == op->getMetaInfos().type) {
					opsFound = eTRUE;
					firstOp = op;
					if(paramNr >= op->getParameterCount()) {
						writeParams = eFALSE;
						break;
					}
					if(_getStaticParam(ds, opType, paramNr)) {
						// ignore
						numStatic++;
						break; // only once per parameter
					} else {
						// store parameter
						paramsWritten++;
						_getOpReport(op).size += _writeParameter(ds, op->getParameter(paramNr));
					}
				}
			}
			paramNr++;

			// break if there are no more ops
			if(!opsFound)
				break;
		}
	}

	// write all pathes
	for(eU32 component = 0; component < 4; component++)
		for(eU32 p = 0; p < m_pathesToStore.size(); p++) 
			_writePath(ds, *m_pathesToStore[p], component);

	// write all scripts
	for(eU32 o = 0; o < m_storedOps.size(); o++) {
		eIOperator * op = m_storedOps[o];
	
		// recompile script without inlined parameters
		eArray<eU32> paramNrs;
		for (eU32 i=0; i<op->getParameterCount(); i++)
			if(!_getStaticParam(ds, op->getMetaInfos().type, i))
				paramNrs.append(i);
        op->compileScript(op->getScript().source, paramNrs);

		// patch ID of referenced path operators in script
		eByteArray patchedScript = op->getScript().byteCode;
		for (eInt i=0; i<=(eInt)patchedScript.size()-4; i++)
		{
			eU8 *ptr = &patchedScript[i];
			for (eU32 j=0; j<m_new2OldOpIds.size(); j++)
				if (*(eU32 *)ptr == m_new2OldOpIds[j])
					*(eU32 *)ptr = _oldToNewOpId(*(eU32 *)ptr);
		}
		eU32 size = 0;
		size += ds.writeU16(op->getScript().memSize);
		size += ds.writeBlob(patchedScript);
		_getOpReport(op).size += size;

		// recompile script again with inlined parameters
        op->compileScript(op->getScript().source);
	}

	eWriteToLog(eString("Inlined ") + eString(eIntToStr(numStatic)) + eString(" parameter types"));
}

void eDemoData::_writeShaders(eDemoScript &ds) {
	// now write shaders
	eByteArray ba;
	for(eU32 i = 0; i < m_usedShaders.size(); i++) {
		for(eU32 k = 0; k < m_usedShaders[i].shaderContent.length(); k++)
			ba.append(m_usedShaders[i].shaderContent.at(k));
		ba.append(0);
	}

/*
	eByteArray baTransformed;
	eBurrowsWheeler bwt;
	bwt.pack(ba, baTransformed);

	eMoveToFront mtf;
	eByteArray baTransformedMTF;
	mtf.pack(baTransformed, baTransformedMTF);

	ds.writeBlob(baTransformedMTF);
*/
//	eWriteToLog(eString("Untransformed vs Transformed vs trans MTF: ") + eString(eIntToStr(ba.size())) + eString(" ") + eString(eIntToStr(baTransformed.size())) + eString(" ") + eString(eIntToStr(baTransformedMTF.size())));
	ds.writeBlob(ba);
//	eWriteToLog(eString("Wrote: [") + eString((const eChar *)&ba[0]) + "]");
}


eIOperator* eDemoData::_getStaticParam(eDemoScript &ds, eU32 opType, eU32 paramNr) {
//	return nullptr; // disabled for now

	eIOperator * firstOp = nullptr;
	eBool allEqual = eTRUE;
	for(eU32 o = 0; o < m_storedOps.size(); o++) {
		eIOperator * op = m_storedOps[o];
		if(opType == op->getMetaInfos().type) {
			if(firstOp) {
				if(!op->getParameter(paramNr).baseValueEquals(firstOp->getParameter(paramNr).getBaseValue())) 
					allEqual = eFALSE;
			} else {
				firstOp = op;
			}
			// parameters that are used in scripts should not be inlined
			if(op->getScriptParamUsage(paramNr) != 0)
				allEqual = eFALSE;
			if(op->getStaticParamsAllowed() == eFALSE)
				allEqual = eFALSE;
		}
	}
	if(firstOp) {
		if(allEqual && 
		   (firstOp->getParameter(paramNr).getType() != ePT_LABEL) &&
		   (firstOp->getParameter(paramNr).getType() != ePT_LINK) &&
		   (firstOp->getParameter(paramNr).getType() != ePT_PATH) &&
		   (firstOp->getMetaInfos().name != "Terrain") &&
		   (firstOp->getMetaInfos().name != "Planet"))
			return firstOp;
		else
			return nullptr;
	}
	return nullptr;
}

void eDemoData::_analyzeOpExport(eDemoScript &ds) {
    for (eU32 i=0; i<m_new2OldOpTypes.size(); i++) {
		eU32 opType = m_new2OldOpTypes[i];
		eBool searchParams = eTRUE;
		eU32 paramNr = 0;
		eBool opsFound = eFALSE;
		while(searchParams) {
			eIOperator * firstOp = nullptr;
			eU32 numOps = 0;

			eBool paramUsedInScript = eFALSE;
			eBool paramWrittenInScript = eFALSE;
			for(eU32 o = 0; o < m_storedOps.size(); o++) {
				eIOperator * op = m_storedOps[o];
				if(opType == op->getMetaInfos().type) {
					firstOp = op;
					opsFound = eTRUE;
					numOps++;
					if(paramNr >= op->getParameterCount()) {
						searchParams = eFALSE;
						break;
					}
					if((op->getScriptParamUsage(paramNr) & eSCRIPT_EXTVAR_WRITTEN) != 0)
						paramWrittenInScript = eTRUE;
					if((op->getScriptParamUsage(paramNr) & eSCRIPT_EXTVAR_USED) != 0)
						paramUsedInScript = eTRUE;
				}
			}

			if(firstOp && (searchParams)) {
				eIOperator* sop = _getStaticParam(ds, opType, paramNr);
				if(sop) {
					eWriteToLog(firstOp->getMetaInfos().category + ":" + firstOp->getMetaInfos().name + "\" param \"" + firstOp->getParameter(paramNr).getName() + "\" is always the same (" + eIntToStr(numOps) + " times) and will be inlined");
					// parameter can be inlined
					ds.setParamUsage(firstOp, paramNr, eFALSE, paramWrittenInScript, opType, i);
				} else {
					// parameter must be passed
					ds.setParamUsage(firstOp, paramNr, eTRUE, paramWrittenInScript, opType, i);
				}
			}

			paramNr++;
			// break if there are no more ops
			if(!opsFound)
				break;
		}
	}
}

eU8 reverseBits(eU8 b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

eU32 eDemoData::_writeParameter(eDemoScript &ds, eParameter &p)
{
    if (p.getType() == ePT_LABEL) // label parameters are of class string
        return 0;

    const eParamValue &val = p.getBaseValue();
    eU32 written = 0;

    if (p.getType() == ePT_LINK) // special ID mapping for links
    {
        eIOperator *linkedOp = findOperator(val.linkedOpId);
        const eID linkedId = (linkedOp ? linkedOp->getResultOp()->getId() : eNOID);
        written += ds.writeU16(_oldToNewOpId(linkedId));
    }
    else if (p.getType() == ePT_FILE)
    {
        eBool success;
        eByteArray fileData = eFile::readAll(val.string, &success);
        written += ds.writeStr(val.string);
        written += ds.writeBlob(fileData);
        if (!success)
            eWriteToLog(eString("The file '")+val.string+"' could not be found!");
    }
    else
    {
        for (eU32 i=0; i<p.getComponentCount(); i++)
        {
            switch (p.getClass())
            {
            case ePC_COL:
                written += ds.writeU8(val.color[i]);
                break;
            case ePC_INT:
                written += ds.writeS32((&val.integer)[i]);
                break;
            case ePC_FLT:
                written += ds.writeF24((&val.flt)[i]);
                break;
            case ePC_PATH:
				m_pathesToStore.append(&val.path);
                break;
            case ePC_STR:
                written += ds.writeStr(val.string);
                break;
            }
        }
    }

    return written;
}

eU32 eDemoData::_writePath(eDemoScript &ds, const ePath4 &path, eU32 component)
{
    eU32 written = 0;
    for (eU32 i=0; i<4; i++)
    {
        const ePath &subPath = path.getSubPath(i);

		eASSERT(path.getSubPath(i).getKeyCount() < 64);
		if(component == 0) 
			written += ds.writeU8((subPath.getLoopMode() << 6) | (path.getSubPath(i).getKeyCount()));
		if(component == 1) 
			for (eU32 j=0; j<subPath.getKeyCount(); j++) 
				written += ds.writeU8(subPath.getKeyByIndex(j).interpol);
		if(component == 2)
			for (eU32 j=0; j<subPath.getKeyCount(); j++) 
				written += ds.writeF24(subPath.getKeyByIndex(j).time);
		if(component == 3)
			for (eU32 j=0; j<subPath.getKeyCount(); j++) 
				written += ds.writeF24(subPath.getKeyByIndex(j).val);
    }
    
    return written;
}

eIOperator * eDemoData::_findOpDuplicate(eIOperator *op)
{
    for (eU32 i=0; i<m_storedOps.size(); i++)
    {
        eIOperator *curOp = m_storedOps[i];
        if (op->getMetaInfos().type == curOp->getMetaInfos().type)
        {
            eBool equal = eTRUE;
            for (eU32 j=0; j<op->getParameterCount() && equal; j++)
                if (!op->getParameter(j).baseValueEquals(curOp->getParameter(j).getBaseValue()))
                    equal = eFALSE;

			// test whether script is equivalent
			equal &= (op->getScript().byteCode == curOp->getScript().byteCode);

            if (equal)
                return curOp;
        }
    }

    return nullptr;
}

eIOperator * eDemoData::_translateEquivalentOp(eIOperator *op) {
    for (eU32 o=0; o<m_equivalentOps.size(); o+=2)
    {
        eIOperator *srcOp = m_equivalentOps[o];
        eIOperator *dstOp = m_equivalentOps[o + 1];
		if(srcOp == op)
			return dstOp;
	}
	return nullptr;
}

void eDemoData::_buildEquivalencyList(eIOperator *op) {
	op = op->getResultOp();
	for (eU32 i=0; i<op->getAboveOpCount(); i++) 
		_buildEquivalencyList(op->getAboveOp(i));
	for (eU32 i=0; i<op->getLinkOutOpCount(); i++) 
		_buildEquivalencyList(op->getLinkOutOp(i));

	eIOperator * equiOp = _translateEquivalentOp(op);
	if(equiOp)
		return; // we already have an equivalent op

	// check if this op is equivalent to some op we already processed
    for (eU32 o=0; o<m_equivalentOps.size(); o+=2)
    {
        eIOperator *curOp = m_equivalentOps[o + 1]->getResultOp();
        if (op->getMetaInfos().type == curOp->getMetaInfos().type)
        {
            eBool equal = eTRUE;
            for (eU32 j=0; j<op->getParameterCount() && equal; j++)
                if (!op->getParameter(j).baseValueEquals(curOp->getParameter(j).getBaseValue()))
                    equal = eFALSE;

			// test whether script is equivalent
			equal &= (op->getScript().byteCode == curOp->getScript().byteCode);

			if(equal) {
				// found a duplicate op, now test if inputs and links are equivalent
				if((curOp->getAboveOpCount() != op->getAboveOpCount()) || (curOp->getLinkOutOpCount() != op->getLinkOutOpCount()))
					continue;
				for (eU32 i=0; i<curOp->getAboveOpCount(); i++) {
					eIOperator *dupInputOp = _translateEquivalentOp(curOp->getAboveOp(i)->getResultOp());
					if(dupInputOp == nullptr) dupInputOp = curOp->getAboveOp(i)->getResultOp();
					eIOperator *equivInputOp = _translateEquivalentOp(op->getAboveOp(i)->getResultOp());
					if(equivInputOp != dupInputOp) {
						equal = eFALSE;
						break;
					}
				}
				if(!equal)
					continue;
				for (eU32 i=0; i<curOp->getLinkOutOpCount(); i++) {
					eIOperator *dupLinkOp = _translateEquivalentOp(curOp->getLinkOutOp(i)->getResultOp());
					if(dupLinkOp == nullptr) dupLinkOp = curOp->getLinkOutOp(i)->getResultOp();
					eIOperator *equivLinkOp = _translateEquivalentOp(op->getLinkOutOp(i)->getResultOp());
					if(equivLinkOp != dupLinkOp) {
						equal = eFALSE;
						break;
					}
				}
				// all tests passed, op is equivalent
				if(equal) {
					m_equivalentOps.append(op);
					m_equivalentOps.append(curOp);
					return;
				}
			}
		}
	}
/**/
	// no equal op found
	m_equivalentOps.append(op);
	m_equivalentOps.append(op);
}

eID eDemoData::_oldToNewOpId(eID opId)
{
    if (opId == eNOID)
        return opId;

    eInt index = m_new2OldOpIds.find(opId);
    if (index == -1)
    {
        m_new2OldOpIds.append(opId);
        index = m_new2OldOpIds.size();
    }

    return index+1;
}

eDemoData::OpExpReport & eDemoData::_getOpReport(eIOperator *op)
{
    const eOpMetaInfos &omi = op->getMetaInfos();

    for (eU32 i=0; i<m_opReps.size(); i++)
        if (m_opReps[i].opType == omi.type)
            return m_opReps[i];

    OpExpReport &oer = m_opReps.append();
    eMemSet(&oer, 0, sizeof(oer));
    oer.opType = omi.type;
    oer.opName = omi.name;
    oer.opCategory = omi.category;
    return oer;
}

eBool eDemoData::_sortOpRepsByRatio(const OpExpReport &oer0, const OpExpReport &oer1)
{
    return (oer0.ratio > oer1.ratio);
}

void eDemoData::_createListOfUsedShaders(eArray<eIOperator*>& ops) {
	// gather a list of all used shaders
	m_usedShaders.clear();
	m_unusedShaders.clear();
	for(eU32 i = 0; i < eEngine::_usedShaders.size(); i++) {
		eShaderDefinition& sd = eEngine::_usedShaders[i];
		// check if the corresponding op-class is used
		eBool isUsed = eFALSE;
		for(eU32 o = 0; o < ops.size(); o++) {
			if(ops[o]->getMetaInfos().className == sd.opClass)
				isUsed = eTRUE;
		}

		if(isUsed || (sd.opClass == "")) {
			eBool alreadyInList = eFALSE;
			for(eU32 k = 0; k < m_usedShaders.size(); k++) {
				if(m_usedShaders[k].name == sd.shaderName)
					alreadyInList = eTRUE;
			}
			if(!alreadyInList) {
				eSHADER_ENTRY& e = m_usedShaders.append();
				e.name = sd.shaderName;
				e.shaderContent = sd.shaderContent;
			}
		};
	}
	// build list of unused shaders
	for(eU32 i = 0; i < eEngine::_usedShaders.size(); i++) {
		eShaderDefinition& sd = eEngine::_usedShaders[i];
		eBool isInUsedList = eFALSE;
		for(eU32 k = 0; k < m_usedShaders.size(); k++)
			if(m_usedShaders[k].name == sd.shaderName)
				isInUsedList = eTRUE;
		if(isInUsedList)
			continue;
		eBool alreadyInList = eFALSE;
		for(eU32 k = 0; k < m_unusedShaders.size(); k++) {
			if(m_unusedShaders[k].name == sd.shaderName)
				alreadyInList = eTRUE;
		}
		if(!alreadyInList) {
			eSHADER_ENTRY& e = m_unusedShaders.append();
			e.name = sd.shaderName;
			e.shaderContent = sd.shaderContent;
		}
	}
}

eU8 eDemoData::_oldToNewOpType(eU32 opType)
{
    eInt index = m_new2OldOpTypes.find(opType);
    eASSERT(index != -1);
    return index+eOPSF_COUNT;
}

#else

eIOpPtrArray	eDemoData::m_ops;
eArray<ePath4*>	eDemoData::m_pathesToRead;
//eArray<eU32> eDemoData::m_new2OldOpTypes;

eIDemoOp * eDemoData::importScript(eDemoScript &ds)
{
    // import script
    const eU32 opType = ds.readU8();
    eID firstOpId = eNOID+1;
    eIDemoOp *demoOp = (eIDemoOp *)_readOperator(ds, opType, firstOpId);
	_readCollectedOps(ds);
//    eASSERT(demoOp->getMetaInfos().type == eOP_TYPE("Misc", "Demo"));
    return demoOp;
}

void eDemoData::free()
{
    for (eU32 i=0; i<m_ops.size(); i++)
        eDelete(m_ops[i]);
}

eBool eDemoData::existsOperator(eID opId)
{
    return (findOperator(opId) != nullptr);
}

eIOperator * eDemoData::findOperator(eID opId)
{
    const eInt index = eIOperator::binarySearchOp(m_ops, opId);
    return (index >= 0 ? m_ops[index] : nullptr);
}

eU32 eDemoData::getTotalOpCount()
{
    return m_ops.size();
}

eIOperator * eDemoData::_readOperator(eDemoScript &ds, eU8 cmd, eID &opId)
{
    // was this operator already read before?
    // (in DAG multiple operators can link the same)
    if (cmd == eOPSF_ALREADY_STORED)
    {
        const eU32 inOpId = ds.readU16();
        return findOperator(inOpId);
    }

    eIOperator *op = nullptr;
    eU32 opType = cmd-eOPSF_COUNT;
    
    // create new operator of given type
    op = eIOperator::newInstance(opType);
    eASSERT(op);
    op->m_id = opId++;
    m_ops.append(op);

    // read all input operators (above and links)
    eBool loadingLinks = eFALSE;
    while (eTRUE)
    {
        const eU32 inCmd = ds.readU8();

        if (inCmd == eOPSF_ABOVE_END)
            loadingLinks = eTRUE;
        else if (inCmd == eOPSF_LINKED_END)
            break;
        else
        {
            eIOperator *inOp = _readOperator(ds, inCmd, opId);

            // add operators altough they might be added
            // multiple times (one operator linking the
            // same operator twice e.g.) as this is crucial
            // for the player memory management to work
            if (loadingLinks)
            {
                op->m_linkOutOps.append(inOp);
                inOp->m_linkInOps.append(op);
            }
            else
            {
                op->m_aboveOps.append(inOp);
                inOp->m_belowOps.append(op);
            }

            op->m_inputOps.append(inOp);
            inOp->m_outputOps.append(op);
        }
    }

    return op;
}

void eDemoData::_readCollectedOps(eDemoScript &ds) {
	eU32 sum = 0;
    for (eU32 i=0; i<ePLAYER_OPTYPE_COUNT; i++) {
		eU32 opType = i;
		eBool readParams = eTRUE;
		eU32 paramNr = 0;
		while(readParams) {
			for(eU32 o = 0; o < m_ops.size(); o++) {
				eIOperator * op = m_ops[o];
				if(opType == op->getMetaInfos().type) {
					if(paramNr >= op->getParameterCount()) {
						readParams = eFALSE;
						break;
					}
					// read parameter data of operator
					_readParameter(ds, op->getParameter(paramNr));
				}
			}
			paramNr++;
		}
	}

	for(eU32 component = 0; component < 4; component++)
		for(eU32 p = 0; p < m_pathesToRead.size(); p++)
			_readPath(ds,*m_pathesToRead[p], component);

	for(eU32 o = 0; o < m_ops.size(); o++) {
		eIOperator * op = m_ops[o];
		op->getScript().memSize = ds.readU16();
		op->getScript().byteCode = ds.readBlob();
	}


}

void eDemoData::readShaders(eDemoScript &ds) {
	// read shaders
//	eEngine::demodata_shaders.reserve(10000000); // make shader array big enough so that no resize occurs
	eEngine::demodata_shaders = ds.readBlob();
	eU32 pos = 0;
	for(eU32 i = 0; i < eSHADERS_COUNT; i++) {
		eEngine::demodata_shader_indices[i] = (const char*)&eEngine::demodata_shaders[pos];
		while(eEngine::demodata_shaders[pos++] != 0);
//		eWriteToLog(eString("Read: [") + eString((const eChar *)&ba[0]) + "]");
	}
}

void eDemoData::_readParameter(eDemoScript &ds, eParameter &p)
{
#ifdef eEDITOR
    eParamValue &val = p.getBaseValue();
#else
    eParamValue &val = p.getAnimValue();
#endif

    if (p.getType() == ePT_LINK)
    {
        val.linkedOpId = ds.readU16();
    } else {
		for (eU32 i=0; i<p.getComponentCount(); i++)
		{
			eASSERT(p.getType() != ePT_LABEL);

			switch (p.getClass())
			{
			case ePC_COL:
				val.color[i] = ds.readU8();
				break;
			case ePC_INT:
				(&val.integer)[i] = ds.readS32();
				break;
			case ePC_FLT:
				(&val.flt)[i] = ds.readF24();
				break;
			case ePC_PATH:
				m_pathesToRead.append(&val.path);
//				_readPath(ds, val.path);
				break;
			case ePC_STR:
				val.string = ds.readStr();
				if (p.getType() == ePT_FILE)
					val.fileData = ds.readBlob();
				break;
			}
		}
	}
#ifdef eEDITOR
	p.getAnimValue() = val;
#endif
}

void eDemoData::_readPath(eDemoScript &ds, ePath4 &path, eU32 component)
{
    for (eU32 i=0; i<4; i++)
    {
        ePath &subPath = path.getSubPath(i);

		if(component == 0) {
			eU8 cmd = ds.readU8();
			subPath = ePath(cmd & 0x3F);
			subPath.setLoopMode((ePathLoopMode)(cmd >> 6));
		}
		for (eU32 j=0; j<subPath.getKeyCount(); j++) {
			ePathKey &key = subPath.getKeyByIndex(j);
			if(component == 1) 
				key.interpol = (ePathKeyInterpol)ds.readU8();
			if(component == 2) 
				key.time = ds.readF24();
			if(component == 3) 
				key.val = ds.readF24();
		}
    }
}

#endif


