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

#ifndef PARAMETER_HPP
#define PARAMETER_HPP

class eDemoScript;
class eIOperator;

enum eParamType
{
    ePT_INT,
    ePT_ENUM,
    ePT_FLAGS,
    ePT_BOOL,
    ePT_STR,
    ePT_TEXT,
    ePT_FLOAT,
    ePT_FXY,
    ePT_FXYZ,
    ePT_FXYZW,
    ePT_RGB,
    ePT_RGBA,
    ePT_IXY,
    ePT_IXYZ,
    ePT_IXYXY,
    ePT_LABEL,
    ePT_LINK,
    ePT_FILE,
    ePT_PATH,
    ePT_COUNT
};

enum eParamClass
{
    ePC_INT,
    ePC_FLT,
    ePC_COL,
    ePC_STR,
    ePC_PATH,
};

struct eParamValue
{
    union
    {
        eInt            integer;
        eF32            flt;
        eInt            enumSel;
        eU8             flags;
        eBool           boolean;
        eFXY            fxy;
        eFXYZ           fxyz;
        eFXYZW          fxyzw;
        eIXY            ixy;
        eIXYZ           ixyz;
        eIXYXY          ixyxy;
        eID             linkedOpId;

        struct
        {
            eU8         flag0 : 1;
            eU8         flag1 : 1;
            eU8         flag2 : 1;
            eU8         flag3 : 1;
            eU8         flag4 : 1;
            eU8         flag5 : 1;
            eU8         flag6 : 1;
            eU8         flag7 : 1;
        };
    };

    eColor              color;
    ePath4              path;
    eString             string;

#ifdef ePLAYER
    eByteArray          fileData;
#endif
};

class eParameter
{
public:
#ifdef ePLAYER
    eParameter(eParamType type, eF32 min, eF32 max, eIOperator *ownerOp);
#else
    eParameter(eParamType type, const eString& varAllocator, const eString& varCaster, const eString& varName, const eString &name, eF32 min, eF32 max, const eParamValue &defVal, eIOperator *ownerOp);

    eBool               baseValueEquals(const eParamValue &val) const;
	eByteArray			baseValueToByteArray() const;

    void                setDescription(const eString &descr);
    void                setAllowedLinks(eInt allowedLinks);
    void                setRequiredLink(eBool requiredLink);

    const eParamValue & getDefaultValue() const;
    const eString &     getDescription() const;
    eInt                getAllowedLinks() const;
    eBool               isRequiredLink() const;
    const eString &     getName() const;
    const eString &     getVarName() const;
    const eString &     getVarAllocator() const;
    const eString &     getVarCaster() const;
#endif

    void                setChanged(eBool reconnect=eFALSE);

    eF32                getMin() const;
    eF32                getMax() const;
    eParamValue &       getAnimValue();
    const eParamValue & getAnimValue() const;
    eParamValue &       getBaseValue();
    const eParamValue & getBaseValue() const;
    eIOperator *        getOwnerOp() const;
    eParamType          getType() const;
    eParamClass         getClass() const;
    eBool               isAnimatable() const;
    eU32                getComponentCount() const;

private:
    const eParamType    m_type;
    eParamValue         m_animVal;
    eF32                m_min;
    eF32                m_max;
    eIOperator *        m_ownerOp;

#ifdef eEDITOR
    eParamValue         m_baseVal;
    const eParamValue   m_defVal;
    eString             m_descr; // used for enum and flags texts
    eInt                m_allowedLinks;
    eString             m_name;
    eString             m_varName;
	eString				m_varAllocator;
	eString				m_varCaster;
    eBool               m_requiredLink; // operator invalid without this link set?
#endif
};

#endif // PARAMETER_HPP