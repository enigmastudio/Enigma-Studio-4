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
const char *eDemoScript::StreamTypeNames[]={ 
		"STREAM_BYTE",
		"STREAM_INT",
		"STREAM_FLT0",
		"STREAM_FLT1",
		"STREAM_FLT2",
		"STREAM_FLT3",
		"STREAM_BLOB"
};
#endif

eDemoScript::eDemoScript(eConstPtr script, eU32 length)
{
#ifdef ePLAYER
    // if a demo script is supposed to be loaded,
    // split the input script into its streams
    eASSERT(length > 0);
    eDataStream stream(script, length);

    for (eU32 i=0; i<STREAM_COUNT; i++)
    {
        const eU32 streamLen  = stream.readU32();
        m_streams[i].attach((eU8 *)script+stream.getReadIndex(), streamLen);
        stream.advance(streamLen);
    }
#endif
}

#ifdef eEDITOR

void eDemoScript::addUsedOp(const eIOperator *op)
{
    // structure operators are not allowed, as they
    // unnecessarily pollute the demo script
    const eString &name = op->getMetaInfos().name;
    eASSERT(name != "Load" && name != "Store" && name != "Nop");

    // operator exists already?
    for (eU32 i=0; i<m_usedOps.size(); i++)
        if (m_usedOps[i]->getMetaInfos().type == op->getMetaInfos().type)
            return; // yes

    // no => add it
    m_usedOps.append(op);
}

// returns the header which contains #defines
// for each operator (used to avoid linking
// unused operators in player)
eString eDemoScript::getUsedOpNames() const
{
    eString usedOpNames, name;
    usedOpNames  = "#ifndef SCRIPT_OPS_HPP\r\n";
    usedOpNames += "#define SCRIPT_OPS_HPP\r\n\r\n";

    for (eU32 i=0; i<m_usedOps.size(); i++)
    {
        const eOpMetaInfos &omi = m_usedOps[i]->getMetaInfos();
        name = omi.category+"_"+omi.name;
        name.makeUpper();

        // replace white spaces and dots with under-scores
        for (eU32 j=0; j<name.length(); j++)
            if (name[j] == ' ' || name[j] == '.')
                name[j] = '_';

        usedOpNames += eString("#define HAVE_OP_")+name+"\r\n";
    }
    
    usedOpNames += "\r\n";
    usedOpNames += "#endif";
    return usedOpNames;
}

void eDemoScript::setParamUsage(const eIOperator *op, eU32 paramNr, eBool isVariable, eBool isWrittenInScript, eU32 oldOpType, eU32 newOpType) {
	if(getParamUsage(op, paramNr))
		return;
	eParamUsageRecord& r = m_paramRecs.append();
	r.op = op;
	r.paramNr = paramNr;
	r.isVariable = isVariable;
	r.isWrittenInScript = isWrittenInScript;
	r.oldOpType = oldOpType;
	r.newOpType = newOpType;
}

eString eDemoScript::getOpParamIfDefs() const
{
    eString ifdefs;
    ifdefs  = "\r\n#ifndef SCRIPT_OP_PARAMS_HPP\r\n";
    ifdefs += "#define SCRIPT_OP_PARAMS_HPP\r\n\r\n";
	for(eU32 i = 0; i < m_paramRecs.size(); i++) {
		const eParamUsageRecord& r = m_paramRecs[i];

		const eParameter& p = r.op->getParameter(r.paramNr);
		eString name = r.op->getMetaInfos().className;
		eString varName = p.getVarName();

		eString parSelector = eString("eOP_SELECT_PAR_") + name + "__" + varName;
		ifdefs += eString("#define ") + parSelector + "(VA) " + p.getVarAllocator() + " " + varName + ",VA\r\n";
		eString parSelectorNone = eString("eOP_SELECTNONE_PAR_") + name + "__" + varName;
		ifdefs += eString("#define ") + parSelectorNone + "(VA) VA\r\n";
		eString parStaticNone = eString("eOP_STATICNONE_PAR_") + name + "__" + varName;
		ifdefs += eString("#define ") + parStaticNone + "(...) __VA_ARGS__\r\n";
	
		eString parAccessMacroName = eString("eOP_ACCESS_PAR_") + name + "__" + eIntToStr(r.paramNr);
		eString parAccessMacro = eString("#define ") + parAccessMacroName + "(OPPTR) (OPPTR->" + varName + ")\r\n";

		eString parDef = eString("PARAMDEF");
		eString parStatic = parStaticNone;

		if(!r.isVariable) {
			eString parValue = eString("eOP_SELECT_PAR_VALUE_") + name + "__" + varName;
			eString valueName = eString(varName) + "_value";
			eByteArray ba = p.baseValueToByteArray();
			eString baString("{");
			for(eU32 i = 0; i < ba.size(); i++) {
				if(i != 0) baString += ",";
				baString += eIntToStr(ba[i]);
			}
			baString += "}";
			eString inlineParamDef(eString("static const eU8 ") + valueName + "[] = " + baString + "; " + p.getVarAllocator() + " " + varName + " = (" + p.getVarCaster() + ")" + valueName + ";");
			switch(p.getType()) {
			case ePT_FLOAT:
				inlineParamDef = eString("const eF32 ") + varName + " = (eF32)" + eFloatToStr(p.getBaseValue().flt) + ";";
				break;
			case ePT_INT:
				inlineParamDef = eString("const eU32 ") + varName + " = " + eIntToStr(p.getBaseValue().integer) + ";";
				break;
			case ePT_BOOL:
				inlineParamDef = eString("const eBool ") + varName + " = " + eIntToStr(p.getBaseValue().boolean) + ";";
				break;
			case ePT_ENUM:
				inlineParamDef = eString("const eInt ") + varName + " = " + eIntToStr(p.getBaseValue().enumSel) + ";";
				break;
			case ePT_FLAGS:
				inlineParamDef = eString("const eInt ") + varName + " = " + eIntToStr(p.getBaseValue().flags) + ";";
				break;
			default:
				;
			};
			ifdefs += eString("#define ") + parValue + "(...) " + inlineParamDef + " __VA_ARGS__\r\n";
			parStatic = parValue;
			parSelector = parSelectorNone;
		}
		parDef = eString("");

		ifdefs += eString("#define eOP_PAR_GLUE_VARARGS_") + name + "__" + varName + "(OPNAME,VARNAME,INITIALIZER,PARAMDEF,REM) " + parSelector + "," + parStatic + "," + parDef + ",REM\r\n";
	}


	// generate an array that holds all parameter initializer information
	eByteArray parray;
	eByteArray pminmaxarray;
	eByteArray pnumarray;
	for(eU32 opType = 0; opType < m_usedOps.size(); opType++) {
		eString	pstr("");
		eU32 pcnt = 0;
		for(eU32 i = 0; i < m_paramRecs.size(); i++) {
			const eParamUsageRecord& r = m_paramRecs[i];
			if(r.newOpType == opType) {
				if(r.isVariable) {
					// all params come in order
					const eParameter& p = r.op->getParameter(r.paramNr);
					if(p.getType() != ePT_LABEL) {
						pcnt++;
//						parray.append(0);			// param indicator
						parray.append(p.getType());	// param type
						if(r.isWrittenInScript) {
							parray.append(1);		// indicator
							eF32 min = p.getMin();
							eF32 max = p.getMax();

							for(eU32 k = 0; k < 4; k++)
								pminmaxarray.append(((eU8*)&min)[k]);
							for(eU32 k = 0; k < 4; k++)
								pminmaxarray.append(((eU8*)&max)[k]);
						} else {
							parray.append(0);		// indicator
						}
					}
				}
			}
		}
		pnumarray.append(pcnt);
//		parray.append(1); // stop
	}
	// build param count array string
	ifdefs += "#define ePAR_COUNT_ARRAY {";
	for(eU32 i = 0; i < pnumarray.size(); i++) {
		if(i != 0) ifdefs += ",";
		ifdefs += eIntToStr(pnumarray[i]);
	}
	ifdefs += "};\r\n";

	// build param array string
	ifdefs += "#define ePAR_INITIALIZER_ARRAY {";
	for(eU32 i = 0; i < parray.size(); i++) {
		if(i != 0) ifdefs += ",";
		ifdefs += eIntToStr(parray[i]);
	}
	ifdefs += "};\r\n";

	// build param minmax array string
	ifdefs += "#define ePAR_MINMAX_ARRAY {";
	for(eU32 i = 0; i < pminmaxarray.size(); i++) {
		if(i != 0) ifdefs += ",";
		ifdefs += eIntToStr(pminmaxarray[i]);
	}
	ifdefs += "};\r\n";

	// write shader indices
	ifdefs += eString("#define eSHADERS_COUNT ") + eString(eIntToStr(eDemoData::m_usedShaders.size())) + eString("\r\n");

	ifdefs += "enum eSHADER_INDICES {";
	for(eU32 i = 0; i < eDemoData::m_usedShaders.size(); i++) {
		if(i != 0) ifdefs += ",";
		ifdefs += eString("eSHADER_IDX_") + eDemoData::m_usedShaders[i].name;
	}
	ifdefs += "};\r\n";

	// we need to define a list for the unused shaders as well
	ifdefs += "enum eSHADER_INDICES_UNUSED {";
	for(eU32 i = 0; i < eDemoData::m_unusedShaders.size(); i++) {
		if(i != 0) ifdefs += ",";
		ifdefs += eString("eSHADER_IDX_") + eDemoData::m_unusedShaders[i].name;
	}
	ifdefs += eString("eSHADER_IDX_dx11infos");
	ifdefs += "};\r\n";


	eString opcreator = "#define eCREATEOP \\\r\n";
	opcreator += "eIOperator* globalCreateOp(eU32 opType) {\\\r\n";
	opcreator += "	switch(opType) {\\\r\n";

	eString opexec = "#define eEXECOP \\\r\n";
	opexec += "ePtr globalOpExecFunction(eU32 opType) {\\\r\n";
	opexec += "	ePtr func = nullptr;\\\r\n";
	opexec += "	switch(opType) {\\\r\n";
//	opexec += eString("	ePtr funcs[") + eString(eIntToStr(eDemoData::m_new2OldOpTypes.size())) + "];\\\r\n";
//	opexec += eString("	__asm { mov ebx, dword ptr [funcs] };\\\r\n");


	// build optype array
    for (eU32 opType=0; opType< eDemoData::m_new2OldOpTypes.size(); opType++) {
		// find one op
		for(eU32 i = 0; i < m_usedOps.size(); i++) {
			const eIOperator* op = m_usedOps[i];
			if(op->getMetaInfos().type == eDemoData::m_new2OldOpTypes[opType]) {
				ifdefs += eString("#define ePLAYER_OPTYPE_") + op->getMetaInfos().className + " " + eString(eIntToStr(opType)) + "\r\n";
				opcreator += eString("	case ") + eString(eIntToStr(opType)) + ":\\\r\n";
				opcreator += eString(" return new ") + eString(op->getMetaInfos().className) + "();\\\r\n";
//				opexec += eString("	__asm { mov eax, dword ptr [") + op->getMetaInfos().className + "::execute] }\\\r\n";
//				opexec += eString("	__asm { mov dword ptr [ebx], eax };\\\r\n");
//				opexec += eString("	__asm { add ebx, 4 };\\\r\n");

				opexec += eString("	case ") + eString(eIntToStr(opType)) + ":\\\r\n";
				opexec += eString("	__asm { mov eax, dword ptr [") + op->getMetaInfos().className + "::execute] }\\\r\n";
				opexec += " break;\\\r\n";
				break;
			}
		}
	}
	opcreator += "	};\\\r\n";
	opcreator += "};\\\r\n";

	opexec += "	};\\\r\n";
	opexec += "__asm { mov dword ptr [func], eax }\\\r\n";
	opexec += "return func;}\r\n";
//	opexec += "return funcs[opType];}\r\n";

	ifdefs += eString("#define ePLAYER_OPTYPE_COUNT ") + eString(eIntToStr(eDemoData::m_new2OldOpTypes.size())) + "\r\n";

	ifdefs += opcreator;
	ifdefs += "extern eIOperator* globalCreateOp(eU32 opType);\r\n";

	ifdefs += opexec;
	ifdefs += "extern ePtr globalOpExecFunction(eU32 opType);\r\n";

    ifdefs += "#endif";
    return ifdefs;
}

const eParamUsageRecord* eDemoScript::getParamUsage(const eIOperator * op, eU32 paramNr) const {
	for(eU32 i = 0; i < m_paramRecs.size(); i++) {
		const eParamUsageRecord& r = m_paramRecs[i];
		if((r.op->getMetaInfos().type == op->getMetaInfos().type) && (r.paramNr == paramNr))
			return &r;
	}
	return nullptr;
}

// returns final demo script by concatenating all
// data streams into one stream. the "format" is
// (length stream 0, data stream 0) - ... -
// (length stream N - data stream N).
eByteArray eDemoScript::getFinalScript()
{
    eDataStream script;

    for (eU32 s=0; s<STREAM_COUNT; s++)
    {
		eStreamRecord& r = m_streamRecords[s];
		eU32 i = r.oldIdx;
        // append stream length
        const eByteArray &data = m_streams[i].getData();
//        eASSERT(data.size() < 0xffff);
        script.writeU32(data.size());

        // append stream data
        for (eU32 j=0; j<data.size(); j++) {
			eU8 value = data[j];
//			value += (eU32)r.stdDev - r.mean;
            script.writeU8(value);
		}
    }

    return script.getData();
}

eBool streamOrderComparator(const eStreamRecord& r0, const eStreamRecord& r1) {
	return r0.stdDev < r1.stdDev;
}
eBool eU32Comparator(const eU32& r0, const eU32& r1) {
	return r0 < r1;
}

eF32 getVariance(eArray<eU32>& data) {
    eF32 Sum = 0;
    eF32 Sum_sqr = 0;
	for(eU32 i = 0; i < data.size(); i++) {
		eF32 x = (eF32)data[i];
        Sum = Sum + x;
        Sum_sqr = Sum_sqr + x*x;
	}
	return (Sum_sqr - ((Sum*Sum)/(eF32)data.size()))/((eF32)data.size() - 1);
}

void eDemoScript::calculateScriptStreamOrder()
{
	m_streamRecords.clear();

	

    for (eU32 i=0; i<STREAM_COUNT; i++)
    {
        // append stream length
        const eByteArray &data = m_streams[i].getData();

		eArray<eU32>	medArray;
		eU32 mean = 0;
		for(eU32 k = 0; k < data.size(); k++) {
			mean += (eU8&)data[k];
			medArray.append(data[k]);
		}
		if(data.size() != 0)
			mean /= data.size();
		medArray.sort(eU32Comparator);
		eU32 median = medArray[medArray.size() >> 1];
		eF32 variance = 0;
		eF32 stdDev = 0;
		if(medArray.size() > 1) {
			variance = getVariance(medArray);
			stdDev = eSqrt(variance);
		}

		eWriteToLog(eString("Stream ") + eString(StreamTypeNames[i]) + ": " + eString(eIntToStr(data.size())) + " bytes - Mean Value: " + eString(eIntToStr(mean)) + " Median Value: " + eString(eIntToStr(median)) + " Std Dev: " + eString(eFloatToStr(stdDev)));
		eStreamRecord r;
		r.oldIdx = i;
		r.mean = mean;
		r.median = median;
		r.variance = variance;
		r.stdDev = stdDev;
		m_streamRecords.append(r);
    }
//	m_streamRecords.sort(streamOrderComparator);

    for (eU32 i=0; i<STREAM_COUNT; i++)
    {
		eStreamRecord& r = m_streamRecords[i];
		eWriteToLog(eString("New: Stream ") + eString(StreamTypeNames[r.oldIdx]) + ": Mean Value: " + eString(eIntToStr(r.mean))+ " Median Value: " + eString(eIntToStr(r.median))+ " StdDev: " + eString(eFloatToStr(r.stdDev)));
	}
}


#endif

eU32 eDemoScript::writeU8(eU8 u8)
{
    m_streams[STREAM_BYTE].writeU8(u8);
    return 1;
}

eU32 eDemoScript::writeU16(eU16 u16)
{
	m_streams[STREAM_INT].writeU_dynamic(u16);
    return 2;
}

eU32 eDemoScript::writeS32(eS32 s32)
{
    return writeU32(*(eU32 *)&s32);
}

eU32 eDemoScript::writeU32(eU32 u32)
{
	m_streams[STREAM_INT].writeU_dynamic(u32);
    return 4;
}

// IEEE 754 floating-point format:
// [1 sign bit | 8 exponent bits | 23 mantinssa bits]
// we save it in a different order:
// [8 exponent bits | 1 sign bit | 15 mantissa bits]
//
// additionally only one byte is stored for 0.0, 1.0 and
// -1.0 by abusing special floats for that (NaN, zero, ...)
#define eFLOATSAVE_BYTE_STARTCODE		0x01
#define eFLOATSAVE_BYTE_STARTSCALE		0.1f
#define eFLOATSAVE_BYTE_NUM_EXP_TESTS	5
#define eFLOATSAVE_INT_STARTCODE		(eFLOATSAVE_BYTE_STARTCODE + eFLOATSAVE_BYTE_NUM_EXP_TESTS) 
#define eFLOATSAVE_INT_STARTSCALE		0.001f
#define eFLOATSAVE_INT_NUM_EXP_TESTS	8
eU32 eDemoScript::writeF24(eF32 f32)
{
    eU32 written = 1;

    if (f32 == 0.0f)
        m_streams[STREAM_FLT0].writeU8(0x00); // 0x00000000 is float +0.0
    else if (f32 == 1.0f)
        m_streams[STREAM_FLT0].writeU8(0xff); // 0xff000000 is float -0.0
	else {
		// test if float can be represented as a fixed-point byte
		for(eU32 k = 0; k < eFLOATSAVE_BYTE_NUM_EXP_TESTS; k++) {
			eF32 curVal = f32;
			eF32 scale = eFLOATSAVE_BYTE_STARTSCALE;
			for(eU32 e = 0; e < k; e++)
				scale *= 10.0f;
			curVal *= scale;
			eF32 curValRounded = eRound(curVal);
			if (eAreFloatsEqual(curVal, curValRounded) && (curValRounded >= -128) && (curValRounded < 128)) {
				m_streams[STREAM_FLT0].writeU8(eFLOATSAVE_BYTE_STARTCODE + k); // very small float, we never reach that
				m_streams[STREAM_BYTE].writeU8((eU8)(curValRounded)); 
				return 2;
			}
		}
		// test if float can be represented as a fixed-point int
		for(eU32 k = 0; k < eFLOATSAVE_INT_NUM_EXP_TESTS; k++) {
			eF32 curVal = f32;
			eF32 scale = eFLOATSAVE_INT_STARTSCALE;
			for(eU32 e = 0; e < k; e++)
				scale *= 10.0f;
			curVal *= scale;
			eF32 curValRounded = eRound(curVal);
			if (eAreFloatsEqual(curVal, curValRounded) && (curValRounded >= -32768) && (curValRounded < 32768)) {
				m_streams[STREAM_FLT0].writeU8(eFLOATSAVE_INT_STARTCODE + k); // very small float, we never reach that
				writeU16((eS16)curValRounded);
				return 3;
			}
		}


		const eU32 v = *(eU32 *)&f32;
		const eU32 sign = (v>>31)&0x01;
		const eU32 exp = (v>>23)&0xff;
		const eU32 mant = (v<<9)>>9;
		const eU32 cutMant = mant>>8;
		const eU16 cutMantSign = (eU16)((cutMant<<1)|sign);

		m_streams[STREAM_FLT0].writeU8(exp);
		m_streams[STREAM_FLT1].writeU8(eLobyte(cutMantSign));
		m_streams[STREAM_FLT2].writeU8(eHibyte(cutMantSign));
		written = 3;
	}
    return written;
}

eU32 eDemoScript::writeF32(eF32 f32)
{
    for (eU32 i=0; i<4; i++)
        m_streams[STREAM_FLT0+i].writeU8(((eU8 *)&f32)[i]);

    return 4;
}

eU32 eDemoScript::writeStr(const eString &str)
{
    for (eU32 i=0; i<str.length(); i++)
        writeU8(str[i]);

    writeU8(0); // null-terminator
    return str.length()+1;
}

eU32 eDemoScript::writeBlob(const eByteArray &blob)
{
    eDataStream &stream = m_streams[STREAM_BLOB];

//    stream.writeU16(blob.size());
//    stream.writeU8(blob.size());
	eU32 writtenBytes = stream.writeU_dynamic(blob.size());

    for (eU32 i=0; i<blob.size(); i++)
        stream.writeU8(blob[i]);

    return blob.size()+writtenBytes;
}

eU8 eDemoScript::readU8()
{
    return m_streams[STREAM_BYTE].readU8();
}

eU16 eDemoScript::readU16()
{
	return (eU16)m_streams[STREAM_INT].readU_dynamic();
}

eS32 eDemoScript::readS32()
{
    const eU32 u32 = readU32();
    return *(eU32 *)&u32;
}

eU32 eDemoScript::readU32()
{
	return m_streams[STREAM_INT].readU_dynamic();
}


// for notes how that function works see writeF24()
eF32 eDemoScript::readF24()
{
    eU8 exp = m_streams[STREAM_FLT0].readU8();

    if (exp == 0x00)
        return 0.0f;
    else if (exp == 0xff)
        return 1.0f;
	else if(exp < (eFLOATSAVE_BYTE_STARTCODE + eFLOATSAVE_BYTE_NUM_EXP_TESTS)) {
		eF32 scale = eFLOATSAVE_BYTE_STARTSCALE;
		for(eU32 e = 0; e < (eU32)(exp - eFLOATSAVE_BYTE_STARTCODE); e++)
			scale *= 10.0f;
		return ((eF32)((eS8)m_streams[STREAM_BYTE].readU8())) / scale;
	} else if(exp < (eFLOATSAVE_INT_STARTCODE + eFLOATSAVE_INT_NUM_EXP_TESTS)) {
		eF32 scale = eFLOATSAVE_INT_STARTSCALE;
		for(eU32 e = 0; e < (eU32)(exp - eFLOATSAVE_INT_STARTCODE); e++)
			scale *= 10.0f;
		return ((eF32)((eS16)readU16())) / scale;
	} else {
		const eU8 lo = m_streams[STREAM_FLT1].readU8();
		const eU8 hi = m_streams[STREAM_FLT2].readU8();
		const eU32 cutMantSign = eMakeWord(lo, hi);
		const eU32 cutMant = cutMantSign>>1;
		const eU32 mant = cutMant<<8;
		const eU32 sign = cutMantSign&0x00000001;
		const eU32 res = (sign<<31)|(exp<<23)|mant;
		return *(eF32 *)&res;
	}
}

eF32 eDemoScript::readF32()
{
    eF32 f32 = 0.0f;
    for (eU32 i=0; i<4; i++)
        ((eU8 *)&f32)[i] = m_streams[STREAM_FLT0+i].readU8();

    return f32;
}

eString eDemoScript::readStr()
{
    eString str;

    while (eTRUE)
    {
        const eChar c = (eChar)readU8();

        if (c != '\0')
            str += c;
        else
            break;
    }

    return str;
}

eByteArray eDemoScript::readBlob()
{
    eDataStream &stream = m_streams[STREAM_BLOB];
    const eU32 size = stream.readU_dynamic();
    eByteArray blob(size);
    
    for (eU32 i=0; i<size; i++)
        blob[i] = stream.readU8();

    return blob;
}

// returns the number of bytes required to store
// the given range without loss
eU32 eDemoScript::_getStorageByteCount(eU64 range) const
{
    eASSERT(range > 0);
    return eRoundToMultiple(eCeil(eLog2((eF32)range)), 8)/8;
}