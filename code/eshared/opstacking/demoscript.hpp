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

#ifndef DEMO_SCRIPT_HPP
#define DEMO_SCRIPT_HPP

#ifdef eEDITOR
struct eParamUsageRecord {
	const eIOperator *	op;
	eU32				paramNr;
	eBool				isVariable;
	eBool				isWrittenInScript;
	eU32				oldOpType;
	eU32				newOpType;
};

struct eStreamRecord {
	eU32 oldIdx;
	eU32 mean;
	eU32 median;
	eF32 variance;
	eF32 stdDev;
};

#endif

// class used for loading and storing the demo script.
// the data values are managed in multiple data streams,
// depending on the data type. this way the final stream
// can be compressed a lot better. additionally, the
// float and integers are encoded differently depending
// on their ranges.
class eDemoScript
{
public:
    eDemoScript(eConstPtr script=nullptr, eU32 length=0);

#ifdef eEDITOR
    void                addUsedOp(const eIOperator *op);
    eString             getUsedOpNames() const;
	eString				getOpParamIfDefs() const;
	const eParamUsageRecord	*	getParamUsage(const eIOperator * op, eU32 paramNr) const;
	void				setParamUsage(const eIOperator * op, eU32 paramNr, eBool isVariable, eBool isWrittenInScript, eU32 oldOpType, eU32 newOpType);

    eByteArray          getFinalScript();
	void				calculateScriptStreamOrder();
#endif

    eU32                writeU8(eU8 u8);
    eU32                writeU16(eU16 u16);
    eU32                writeS32(eS32 s32);
    eU32                writeU32(eU32 u32);
    eU32                writeF32(eF32 f32);
    eU32                writeF24(eF32 f32);
    eU32                writeStr(const eString &str);
    eU32                writeBlob(const eByteArray &blob);

    eU8                 readU8();
    eU16                readU16();
    eS32                readS32();
    eU32                readU32();
    eF32                readF32();
    eF32                readF24();
    eString             readStr();
    eByteArray          readBlob();

private:
    eU32                _getStorageByteCount(eU64 range) const;

private:

    enum StreamType
    {
        STREAM_BYTE,
        STREAM_INT,
        STREAM_FLT0,
        STREAM_FLT1,
        STREAM_FLT2,
        STREAM_FLT3,
        STREAM_BLOB,
        STREAM_COUNT
    };
#ifdef eEDITOR
	static const char *StreamTypeNames[STREAM_COUNT]; // put all stream names in here
#endif

private:
    eDataStream         m_streams[STREAM_COUNT];

#ifdef eEDITOR
    eIOpConstPtrArray			m_usedOps;
	eArray<eParamUsageRecord>	m_paramRecs;
	
	eArray<eStreamRecord>		m_streamRecords;
#endif
};



#endif // DEMO_SCRIPT_HPP