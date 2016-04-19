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

struct ParameterInfo
{
    eU32        numComponents;
    eBool       animatable;
    eParamClass classs; // three 's' because 'class' is keyword
}
PARAM_INFOS[] =
{
    {1, eTRUE,  ePC_INT},  // int
    {1, eFALSE, ePC_INT},  // enum
    {1, eFALSE, ePC_INT},  // flags
    {1, eFALSE, ePC_INT},  // bool
    {1, eFALSE, ePC_STR},  // string
    {1, eFALSE, ePC_STR},  // text
    {1, eTRUE,  ePC_FLT},  // float
    {2, eTRUE,  ePC_FLT},  // fxy
    {3, eTRUE,  ePC_FLT},  // fxyz
    {4, eTRUE,  ePC_FLT},  // fxyzw
    {3, eTRUE,  ePC_COL},  // rgb
    {4, eTRUE,  ePC_COL},  // rgba
    {2, eTRUE,  ePC_INT},  // ixy
    {3, eTRUE,  ePC_INT},  // ixyz
    {4, eTRUE,  ePC_INT},  // ixyxy
    {1, eFALSE, ePC_STR},  // label
    {1, eFALSE, ePC_INT},  // link
    {1, eFALSE, ePC_STR},  // file
    {1, eFALSE, ePC_PATH}, // path
};

#ifdef ePLAYER
eParameter::eParameter(eParamType type, eF32 min, eF32 max, eIOperator *ownerOp) :
    m_type(type),
    m_min(min),
    m_max(max),
    m_ownerOp(ownerOp)
{
}
#else
eParameter::eParameter(eParamType type, const eString& varAllocator, const eString& varCaster, const eString& varName, const eString &name, eF32 min, eF32 max,
                       const eParamValue &defVal, eIOperator *ownerOp) :
    m_type(type),
    m_min(min),
    m_max(max),
    m_name(name),
	m_varName(varName),
	m_varAllocator(varAllocator),
	m_varCaster(varCaster),
    m_animVal(defVal),
    m_baseVal(defVal),
    m_defVal(defVal),
    m_ownerOp(ownerOp),
    m_allowedLinks(eOC_ALL),
    m_requiredLink(eFALSE)
{
}

// returns if two parameter values differ. as type the
// parameter's type is taken. this is used to decide if a
// parameter has to be stored when exporting the intro script.
eBool eParameter::baseValueEquals(const eParamValue &val) const
{
    switch (getClass())
    {
    case ePC_STR:
        return (m_baseVal.string == val.string);
    case ePC_COL:
        return (m_baseVal.color == val.color);
    case ePC_PATH:
        return (m_baseVal.path == val.path);
    default:
        return eMemEqual(&m_baseVal, &val, 4*getComponentCount());
    }
}

eByteArray eParameter::baseValueToByteArray() const {
	eByteArray result;
    switch (getClass())
    {
    case ePC_STR:
		for(eU32 i = 0; i < m_baseVal.string.length(); i++)
			result.append(m_baseVal.string.at(i));
		result.append(0);
		break;
    case ePC_COL:
		for(eU32 i = 0; i < sizeof(m_baseVal.color); i++)
			result.append(((eU8*)&m_baseVal.color)[i]);
		break;
    case ePC_PATH:
        return result;
    default:
		for(eU32 i = 0; i < 4*getComponentCount(); i++) 
			result.append(((eU8*)&m_baseVal)[i]);
    }
	return result;
}

void eParameter::setDescription(const eString &descr)
{
    eASSERT(m_type == ePT_ENUM || m_type == ePT_FLAGS);
    eASSERT(descr.length() > 0);
    m_descr = descr;
}

void eParameter::setAllowedLinks(eInt allowedLinks)
{
    m_allowedLinks = allowedLinks;
}

void eParameter::setRequiredLink(eBool requiredLink)
{
    m_requiredLink = requiredLink;
}

const eParamValue & eParameter::getDefaultValue() const
{
    return m_defVal;
}

const eString & eParameter::getDescription() const
{
    return m_descr;
}

eInt eParameter::getAllowedLinks() const
{
    return m_allowedLinks;
}

eBool eParameter::isRequiredLink() const
{
    return m_requiredLink;
}

const eString & eParameter::getName() const
{
    return m_name;
}

const eString & eParameter::getVarName() const
{
    return m_varName;
}

const eString & eParameter::getVarAllocator() const
{
    return m_varAllocator;
}

const eString & eParameter::getVarCaster() const
{
    return m_varCaster;
}

#endif

void eParameter::setChanged(eBool reconnect)
{
#ifdef eEDITOR
    m_animVal = m_baseVal;
#endif
    m_ownerOp->setChanged(reconnect);
}

eF32 eParameter::getMin() const
{
    return m_min;
}

eF32 eParameter::getMax() const
{
    return m_max;
}

eParamType eParameter::getType() const
{
    return m_type;
}

eIOperator * eParameter::getOwnerOp() const
{
    return m_ownerOp;
}

eParamValue & eParameter::getAnimValue()
{
    return m_animVal;
}

const eParamValue & eParameter::getAnimValue() const
{
    return m_animVal;
}

#ifdef eEDITOR
eParamValue & eParameter::getBaseValue()
{
    return m_baseVal;
}

const eParamValue & eParameter::getBaseValue() const
{
    return m_baseVal;
}
#endif

eParamClass eParameter::getClass() const
{
    return PARAM_INFOS[m_type].classs;
}

eBool eParameter::isAnimatable() const
{
    return PARAM_INFOS[m_type].animatable;
}

eU32 eParameter::getComponentCount() const
{
    return PARAM_INFOS[m_type].numComponents;
}