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

#ifndef OP_MACROS_HPP
#define OP_MACROS_HPP

#ifdef eEDITOR
	#define eDEF_OPERATOR_SOURCECODE_INTERNAL(CLASSNAME)				\
		class CLASSNAME {												\
		public:																\
				CLASSNAME() {											\
					eDemoData::m_operatorSourceFiles.append(__FILE__);        \
				}															\
			};																\
			CLASSNAME temp;

	#define GLUE_OPSOURCE_NAME(A,B) eDEF_OPERATOR_SOURCECODE_INTERNAL(A##B)
	#define GLUE_OPSOURCE_NAME0(A,B) GLUE_OPSOURCE_NAME(A,B)
	#define eDEF_OPERATOR_SOURCECODE(A)	GLUE_OPSOURCE_NAME0(OPSOURCEFILE,A)

#else
	#define eDEF_OPERATOR_SOURCECODE(A)
#endif

// create an operator type as a compile-time
// hash based on the category and name
#define eOP_TYPE(category, name) (eHashStr(category)^eHashStr(name))

// use these macros if many inputs have to
// be declared (mostly used for merge operators)
#define eOP_4INPUTS(opClass)  opClass, opClass, opClass, opClass
#define eOP_8INPUTS(opClass)  eOP_4INPUTS(opClass), eOP_4INPUTS(opClass)
#define eOP_16INPUTS(opClass) eOP_8INPUTS(opClass), eOP_8INPUTS(opClass)
#define eOP_32INPUTS(opClass) eOP_16INPUTS(opClass), eOP_16INPUTS(opClass)

#ifdef eEDITOR
#define eOP_REGISTER_EFFECT_VAR(opclass, var)																	\
	eEngine::_addEffectVar(#opclass, var)
#else
#define eOP_REGISTER_EFFECT_VAR(opclass, var)                                                              
#endif


#define eOP_INPUTS(...)																					\
	{ __VA_ARGS__ }

#ifdef eEDITOR
#define eOP_DEF(classT, baseClassT, name, category, color, shortcut, output, minAbove, maxAbove, inputs)   \
    static eOpMetaInfos classT##MetaInfos =                                                             \
    {                                                                                                   \
		#classT, name, category, color, shortcut, minAbove, maxAbove, inputs,                           \
        output, eOP_TYPE(category, name), nullptr, nullptr                                              \
    };                                                                                                  \
                                                                                                        \
    class classT : public baseClassT                                                                    \
    {                                                                                                    \
    public:                                                                                             \
        classT()                                                                                        \
        {                                                                                               \
            m_metaInfos = &classT##MetaInfos;                                                           \
            m_width = eClamp<eU32>(1, (eU32)maxAbove, 3)*4;                                             \
			_setupParams();																				\
            _initialize();                                                                              \
        }
#else

#define eOP_DEF(classT, baseClassT, name, category, color, shortcut, output, minAbove, maxAbove, inputs)   \
    static eOpMetaInfos classT##MetaInfos =                                                             \
    {                                                                                                   \
		output, ePLAYER_OPTYPE_##classT																	\
    };                                                                                                  \
                                                                                                        \
    class classT : public baseClassT                                                                    \
    {                                                                                                    \
    public:                                                                                             \
        classT()                                                                                        \
        {                                                                                               \
            m_metaInfos = &classT##MetaInfos;                                                           \
			_setupParams();																				\
            _initialize();                                                                              \
        }
#endif

#ifdef eEDITOR
#define eOP_END(classT)                                                                                 \
        virtual ~classT()                                                                               \
        {                                                                                               \
            _deinitialize();                                                                            \
        }                                                                                               \
                                                                                                        \
        static eIOperator * classT##CreateInstance()                                                    \
        {                                                                                               \
            return new classT;                                                                          \
        }                                                                                               \
																										\
        static const struct RegisterExecFunc                                                            \
        {                                                                                               \
            RegisterExecFunc()                                                                          \
            {                                                                                           \
                ePtr func = nullptr;                                                                    \
                __asm { mov eax, dword ptr [classT::execute] }                                          \
                __asm { mov dword ptr [func], eax }                                                     \
                classT##MetaInfos.execFunc = func;                                                      \
                classT##MetaInfos.createOp = classT##CreateInstance;                                    \
                getAllMetaInfos().append(&classT##MetaInfos);                                           \
            }                                                                                           \
        }                                                                                               \
        regExecFunc;                                                                                    \
    };                                                                                                  \
                                                                                                        \
    const classT::RegisterExecFunc classT::regExecFunc;													\


#else
#define eOP_END(classT)                                                                                 \
        virtual ~classT()                                                                               \
        {                                                                                               \
            _deinitialize();                                                                            \
        }                                                                                               \
                                                                                                        \
    };                                                                                                  \

#endif

#define eOP_EXEC                                                                                        \
   public:                                                                                              \
       void execute


#define eOP_INIT                                                                                        \
   private:                                                                                                \
       void _initialize

#define eOP_DEINIT                                                                                      \
    private:                                                                                            \
        void _deinitialize

#ifdef eEDITOR

#define eOP_INTERACT                                                                                    \
    public:                                                                                             \
        virtual eBool doEditorInteraction
#else
#define eOP_INTERACT																					\
    public:                                                                                             \
        eBool __UNUSED_INTERACTION_FUNCTION
#endif

#define eOP_VAR(var)                                                                                    \
    private:                                                                                            \
        var

#define eOP_DEF_BMP(classT, name, shortcut, minAbove, maxAbove, inputs)                                    \
    eOP_DEF(classT, eIBitmapOp, name, "Bitmap", eColor(185, 230, 115),                                  \
            shortcut, eOC_BMP, minAbove, maxAbove, inputs)

#define eOP_DEF_FX(classT, name, shortcut, minAbove, maxAbove, inputs)                                     \
    eOP_DEF(classT, eIEffectOp, name, "Effect", eColor(128, 0, 128),                                    \
            shortcut, eOC_FX, minAbove, maxAbove, inputs)

#define eOP_DEF_MESH(classT, name, shortcut, minAbove, maxAbove, inputs)                                   \
    eOP_DEF(classT, eIMeshOp, name, "Mesh", eColor(116, 160, 173),                                      \
            shortcut, eOC_MESH, minAbove, maxAbove, inputs)

#define eOP_DEF_SEQ(classT, name, shortcut, minAbove, maxAbove, inputs)                                    \
    eOP_DEF(classT, eISequencerOp, name, "Sequencer", eColor(255, 128, 0),                              \
            shortcut, eOC_SEQ, minAbove, maxAbove, inputs)

#define eOP_DEF_MODEL(classT, name, shortcut, minAbove, maxAbove, inputs)                                  \
    eOP_DEF(classT, eIModelOp, name, "Model", eColor(215, 215, 0),                                      \
            shortcut, eOC_MODEL, minAbove, maxAbove, inputs)

#ifdef eEDITOR

// nulls PODs but not string and path as they have ctors
#define eOP_PARAM_ADD(type, allocator, caster, varname, name, var, min, max, value)                                                 \
{                                                                                                       \
    eParamValue defVal;                                                                                 \
    eMemSet(&defVal, 0, 4*4);                                                                           \
    defVal.var = value;                                                                                 \
	m_params.append(new eParameter(type, #allocator, #caster, #varname, name, min, max, defVal, this));                                \
}

#define eOP_PARAM_ADD_LINK(caster, varname, name, allowedLinks, requiredLink)                                            \
	eOP_PARAM_ADD(ePT_LINK, caster, caster, varname, name, linkedOpId, 0, 0, eNOID);                                             \
    m_params.last()->setAllowedLinks(allowedLinks);                                                     \
    m_params.last()->setRequiredLink(requiredLink);

#define eOP_PARAM_ADD_ENUM(varname, name, descr, index)                                                          \
    eOP_PARAM_ADD(ePT_ENUM, const eInt, const eInt&, varname, name, enumSel, 0, 255, index);                                              \
    m_params.last()->setDescription(descr);

#define eOP_PARAM_ADD_FLAGS(varname, name, descr, index)                                                         \
    eOP_PARAM_ADD(ePT_FLAGS, const eInt, const eInt&, varname, name, flags, 0, 255, index);                                               \
    m_params.last()->setDescription(descr);

#define eOP_PARAM_ADD_LABEL(varname, name, caption)                                                              \
    eOP_PARAM_ADD(ePT_LABEL, NOTYPE, NOTYPE, varname, name, string, 0, 0, caption)

#elif ePLAYER

// nulls PODs but not string and path as they have ctors
#define eOP_PARAM_ADD(type, allocator, caster, varname, name, var, min, max, value)                                                 \
{                                                                                                       \
    m_params.append(new eParameter(type, min, max, this));												\
}

#define eOP_PARAM_ADD_LINK(caster, varname, name, allowedLinks, requiredLink)                                            \
    eOP_PARAM_ADD(ePT_LINK, NONE, NONE, varname, name, linkedOpId, 0, 0, eNOID);                                             \

#define eOP_PARAM_ADD_ENUM(varname, name, descr, index)                                                          \
    eOP_PARAM_ADD(ePT_ENUM, NONE, NONE, varname, name, enumSel, 0, 255, index);                                              \

#define eOP_PARAM_ADD_FLAGS(varname, name, descr, index)                                                         \
    eOP_PARAM_ADD(ePT_FLAGS, NONE, NONE, varname, name, flags, 0, 255, index);                                               \

#define eOP_PARAM_ADD_LABEL(varname, name, caption)                                                              \

#endif

#define eOP_PARAM_ADD_INT(varname, name, min, max, value)                                                        \
    eOP_PARAM_ADD(ePT_INT, const eU32, const eU32&, varname, name, integer, min, max, value)

#define eOP_PARAM_ADD_FLOAT(varname, name, min, max, value)                                                      \
    eOP_PARAM_ADD(ePT_FLOAT, const eF32, const eF32&, varname, name, flt, min, max, value)

#define eOP_PARAM_ADD_FXY(varname, name, min, max, x, y)                                                         \
    eOP_PARAM_ADD(ePT_FXY, const eVector2&, const eVector2&, varname, name, fxy, min, max, eVector2(x, y))

#define eOP_PARAM_ADD_FXYZ(varname, name, min, max, x, y, z)                                                     \
    eOP_PARAM_ADD(ePT_FXYZ, const eVector3&, const eVector3&, varname, name, fxyz, min, max, eVector3(x, y, z))

#define eOP_PARAM_ADD_FXYZW(varname, name, min, max, x, y, z, w)                                                 \
    eOP_PARAM_ADD(ePT_FXYZW, const eVector4&, const eVector4&, varname, name, fxyzw, min, max, eVector4(x, y, z, w))

#define eOP_PARAM_ADD_RGB(varname, name, r, g, b)                                                                \
    eOP_PARAM_ADD(ePT_RGB, const eColor&, const eColor&, varname, name, color, 0, 255, eColor(r, g, b, 255))

#define eOP_PARAM_ADD_RGBA(varname, name, r, g, b, a)                                                            \
    eOP_PARAM_ADD(ePT_RGBA, const eColor&, const eColor&, varname, name, color, 0, 255, eColor(r, g, b, a))

#define eOP_PARAM_ADD_IXY(varname, name, min, max, x, y)                                                         \
    eOP_PARAM_ADD(ePT_IXY, const ePoint&, const ePoint&, varname, name, ixy, min, max, ePoint(x, y))

#define eOP_PARAM_ADD_IXYZ(varname, name, min, max, x, y, z)                                                     \
    eOP_PARAM_ADD(ePT_IXYZ, const eIXYZ&, const eIXYZ&, varname, name, ixyxy, min, max, eRect(x, y, z, 0))

#define eOP_PARAM_ADD_IXYXY(varname, name, min, max, x0, y0, x1, y1)                                             \
    eOP_PARAM_ADD(ePT_IXYXY, const eIXYXY&, const eIXYXY&, varname, name, ixyxy, min, max, eRect(x0, y0, x1, y1))

#define eOP_PARAM_ADD_BOOL(varname, name, state)                                                                 \
    eOP_PARAM_ADD(ePT_BOOL, const eBool, const eBool&, varname, name, boolean, 0, 1, state)

#define eOP_PARAM_ADD_STRING(varname, name, str)                                                                 \
    eOP_PARAM_ADD(ePT_STR, const eChar*, const eChar*, varname, name, string, 0, 0, str)

#define eOP_PARAM_ADD_TEXT(varname, name, text)                                                                  \
    eOP_PARAM_ADD(ePT_TEXT, const eChar*, const eChar*, varname, name, string, 0, 0, text)

#define eOP_PARAM_ADD_FILE(varname, name, file)                                                                  \
    eOP_PARAM_ADD(ePT_FILE, const eChar*, const eChar*, varname, name, string, 0, 0, file)

#define eOP_PARAM_ADD_PATH(varname, name)                                                                        \
    eOP_PARAM_ADD(ePT_PATH, const ePath4&, const ePath4&, varname, name, path, 0, 0, ePath4())



// Black Macro magic starts here

/* And now, helper macrology up the wazoo. 
255   /*
256    * Count the number of arguments passed to eOP_EXEC2, very carefully
257    * tiptoeing around an MSVC bug where it improperly expands __VA_ARGS__ as a
258    * single token in argument lists.  See these URLs for details:
259    *
260    *   http://connect.microsoft.com/VisualStudio/feedback/details/380090/variadic-macro-replacement
261    *   http://cplusplus.co.il/2010/07/17/variadic-macro-to-count-number-of-arguments/#comment-644
262    */

#define VA_NARGS_IMPL2(_1,_2,_3,_4,_5,_6_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,_64,N,...) N
#define VA_NARGS_IMPL1(tuple) VA_NARGS_IMPL2 tuple
#define VA_NARGS(...) VA_NARGS_IMPL1((__VA_ARGS__, 64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0))

#define _eGLUE_PARAMSSEPARATE(x, y) x,y
#define _eGLUE_PARAMSCONCAT(x, y) x y
#define _eGLUE_TOKENCONCAT(x, y) x y

#ifdef eEDITOR
#define eGLUE_FUNC_PARAMS2(...)		
#define eGLUE_FUNC_PARAMS5(INITIALIZER, DUMMY, DEFINITION, END)			INITIALIZER
#define eGLUE_FUNC_PARAMS8(INITIALIZER, DUMMY, DEFINITION, ...)			_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS5(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS11(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS8(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS14(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS11(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS17(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS14(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS20(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS17(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS23(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS20(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS26(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS23(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS29(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS26(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS32(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS29(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS35(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS32(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS38(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS35(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS41(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS38(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS44(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS41(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS47(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS44(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS50(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS47(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS53(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS50(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS56(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS53(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS59(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS56(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS62(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS59(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS65(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS62(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS68(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS65(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS71(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS68(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS74(INITIALIZER, DUMMY, DEFINITION, ...)		_eGLUE_PARAMSSEPARATE(INITIALIZER, eGLUE_FUNC_PARAMS71(__VA_ARGS__))

#else
#define eGLUE_FUNC_PARAMS2(...)		
#define eGLUE_FUNC_PARAMS5(INITIALIZER, DUMMY, DEFINITION, END)			INITIALIZER(eU32 __dummy__)
#define eGLUE_FUNC_PARAMS8(INITIALIZER, DUMMY, DEFINITION, ...)			INITIALIZER(eGLUE_FUNC_PARAMS5(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS11(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS8(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS14(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS11(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS17(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS14(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS20(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS17(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS23(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS20(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS26(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS23(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS29(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS26(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS32(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS29(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS35(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS32(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS38(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS35(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS41(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS38(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS44(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS41(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS47(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS44(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS50(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS47(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS53(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS50(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS56(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS53(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS59(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS56(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS62(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS59(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS65(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS62(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS68(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS65(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS71(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS68(__VA_ARGS__))
#define eGLUE_FUNC_PARAMS74(INITIALIZER, DUMMY, DEFINITION, ...)		INITIALIZER(eGLUE_FUNC_PARAMS71(__VA_ARGS__))

#define eGLUE_STATIC_PARAMS2(...)		
#define eGLUE_STATIC_PARAMS5(DUMMY, STATICDEF, DEFINITION, END)			STATICDEF()
#define eGLUE_STATIC_PARAMS8(DUMMY, STATICDEF, DEFINITION, ...)			STATICDEF(eGLUE_STATIC_PARAMS5(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS11(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS8(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS14(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS11(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS17(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS14(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS20(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS17(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS23(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS20(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS26(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS23(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS29(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS26(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS32(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS29(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS35(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS32(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS38(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS35(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS41(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS38(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS44(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS41(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS47(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS44(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS50(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS47(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS53(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS50(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS56(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS53(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS59(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS56(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS62(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS59(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS65(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS62(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS68(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS65(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS71(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS68(__VA_ARGS__))
#define eGLUE_STATIC_PARAMS74(DUMMY, STATICDEF, DEFINITION, ...)		STATICDEF(eGLUE_STATIC_PARAMS71(__VA_ARGS__))

#endif

#define eGLUE_SETUP_PARAMS2(...)		
#define eGLUE_SETUP_PARAMS5(INITIALIZER, DEFVAL, DEFINITION, END)				DEFINITION
#define eGLUE_SETUP_PARAMS8(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS5(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS11(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS8(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS14(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS11(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS17(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS14(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS20(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS17(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS23(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS20(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS26(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS23(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS29(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS26(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS32(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS29(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS35(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS32(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS38(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS35(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS41(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS38(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS44(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS41(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS47(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS44(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS50(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS47(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS53(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS50(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS56(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS53(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS59(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS56(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS62(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS59(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS65(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS62(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS68(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS65(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS71(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS68(__VA_ARGS__))
#define eGLUE_SETUP_PARAMS74(INITIALIZER, DEFVAL, DEFINITION, ...)		_eGLUE_TOKENCONCAT(DEFINITION, eGLUE_SETUP_PARAMS71(__VA_ARGS__))


#define CHOOSE_FUNC_HELPER2(count) eGLUE_FUNC_PARAMS##count
#define CHOOSE_FUNC_HELPER1(count) CHOOSE_FUNC_HELPER2(count)
#define CHOOSE_FUNC_HELPER(count) CHOOSE_FUNC_HELPER1(count)



#define ENABLE_STATIC_PARAMS eTRUE
#define DISABLE_STATIC_PARAMS eFALSE

#define eGLUE_STATIC_PARAMS(...)
#ifdef eEDITOR
#define CHOOSE_STATIC_HELPER(count) eGLUE_STATIC_PARAMS
#define _ePAR_MAKESTATIC(VAL)	m_allowStaticParameters = VAL;
#define _ePAR_MAKEPARDECLARE	
#else
#define CHOOSE_STATIC_HELPER2(count) eGLUE_STATIC_PARAMS##count
#define CHOOSE_STATIC_HELPER1(count) CHOOSE_STATIC_HELPER2(count)
#define CHOOSE_STATIC_HELPER(count) CHOOSE_STATIC_HELPER1(count)
#define _ePAR_MAKESTATIC(VAL)	
#define _ePAR_MAKEPARDECLARE	setupParameterDefinitions()
#endif

#define CHOOSE_SETUP_HELPER2(count) eGLUE_SETUP_PARAMS##count
#define CHOOSE_SETUP_HELPER1(count) CHOOSE_SETUP_HELPER2(count)
#define CHOOSE_SETUP_HELPER(count) CHOOSE_SETUP_HELPER1(count)

#define eOP_EXEC2(STATIC_ALLOWED, ...)																	\
   public:                                                                                              \
	   void _setupParams() {																			\
			_ePAR_MAKESTATIC(STATIC_ALLOWED)															\
			_ePAR_MAKEPARDECLARE;																		\
			_eGLUE_PARAMSCONCAT(CHOOSE_SETUP_HELPER(VA_NARGS(__VA_ARGS__)), (__VA_ARGS__))				\
	   }																								\
	   void execute(_eGLUE_PARAMSCONCAT(CHOOSE_FUNC_HELPER(VA_NARGS(__VA_ARGS__)), (__VA_ARGS__)))		\
	   {																								\
			_eGLUE_PARAMSCONCAT(CHOOSE_STATIC_HELPER(VA_NARGS(__VA_ARGS__)), (__VA_ARGS__))				


#define eOP_EXEC2_END																					\
		}


#define eOP_PAR_END dummy
//#define eOP_PAR_GLUE_INITIALIZER(INITIALIZER) _eGLUE_PARAMSSEPARATE(INITIALIZER, 
#define eOP_PAR_GLUE1(a) a
//#define eOP_PAR_GLUE(VARNAME,INITIALIZER,DEFVAL,REM) INITIALIZER VARNAME,INITIALIZER VARNAME = DEFVAL,REM
//#define eOP_PAR_GLUE(VARNAME,INITIALIZER,DEFVAL,REM) INITIALIZER VARNAME,INITIALIZER VARNAME = DEFVAL,REM

#define eOP_PAR_SELECTOR_DEFAULT(INITIALIZER, VARARGS) _eGLUE_PARAMSSEPARATE(INITIALIZER, VARARGS)
#define eOP_PAR_GLUE_VARARGS(OPNAME,VARNAME,INITIALIZER,PARAMDEF,REM)											\
	INITIALIZER VARNAME, ,PARAMDEF,REM

//	#define eOP_SELECT_PAR_##OPNAME##__##VARNAME(VA) _eGLUE_PARAMSSEPARATE(INITIALIZER VARNAME, VA)				\
//	INITIALIZER VARNAME,eOP_SELECT_PAR_##OPNAME##__##VARNAME,PARAMDEF,REM

#ifdef eEDITOR
#define eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME) eOP_PAR_GLUE_VARARGS
#else 
#define eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME) eOP_PAR_GLUE_VARARGS_##OPNAME##__##VARNAME
#endif

#define eOP_PAR_RGB(OPNAME, VARNAME, name, r, g, b, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eColor&, eOP_PARAM_ADD_RGB(VARNAME, name, r, g, b), __VA_ARGS__)
#define eOP_PAR_RGBA(OPNAME, VARNAME, name, r, g, b, a, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eColor&, eOP_PARAM_ADD_RGBA(VARNAME, name, r, g, b, a), __VA_ARGS__)
#define eOP_PAR_PATH(OPNAME, VARNAME, name, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const ePath4&, eOP_PARAM_ADD_PATH(VARNAME, name), __VA_ARGS__)
#define eOP_PAR_BOOL(OPNAME, VARNAME, name, state, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eBool, eOP_PARAM_ADD_BOOL(VARNAME, name, state), __VA_ARGS__)
#define eOP_PAR_FILE(OPNAME, VARNAME, name, file, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eChar*, eOP_PARAM_ADD_FILE(VARNAME, name, file), __VA_ARGS__)
#define eOP_PAR_STRING(OPNAME, VARNAME, name, str, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eChar*, eOP_PARAM_ADD_STRING(VARNAME, name, str), __VA_ARGS__)
#define eOP_PAR_TEXT(OPNAME, VARNAME, name, str, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eChar*, eOP_PARAM_ADD_TEXT(VARNAME, name, str), __VA_ARGS__)
#define eOP_PAR_FLAGS(OPNAME, VARNAME, name, descr, index, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eInt, eOP_PARAM_ADD_FLAGS(VARNAME, name, descr, index), __VA_ARGS__)
#define eOP_PAR_ENUM(OPNAME, VARNAME, name, descr, index, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eInt, eOP_PARAM_ADD_ENUM(VARNAME, name, descr, index), __VA_ARGS__)
#define eOP_PAR_INT(OPNAME, VARNAME, name, min, max, x, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eU32, eOP_PARAM_ADD_INT(VARNAME, name, min, max, x), __VA_ARGS__)
#define eOP_PAR_IXY(OPNAME, VARNAME, name, min, max, x, y, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const ePoint &, eOP_PARAM_ADD_IXY(VARNAME, name, min, max, x, y), __VA_ARGS__)
#define eOP_PAR_IXYZ(OPNAME, VARNAME, name, min, max, x, y, z, ...)																		\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eIXYZ &, eOP_PARAM_ADD_IXYZ(VARNAME, name, min, max, x, y,z), __VA_ARGS__)
#define eOP_PAR_IXYXY(OPNAME, VARNAME, name, min, max, x, y, z, w, ...)																		\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eIXYXY &, eOP_PARAM_ADD_IXYXY(VARNAME, name, min, max, x, y, z, w), __VA_ARGS__)
#define eOP_PAR_FLOAT(OPNAME, VARNAME, name, min, max, x, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eF32, eOP_PARAM_ADD_FLOAT(VARNAME, name, min, max, x), __VA_ARGS__)
#define eOP_PAR_FXY(OPNAME, VARNAME, name, min, max, x, y, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eVector2 &, eOP_PARAM_ADD_FXY(VARNAME, name, min, max, x, y), __VA_ARGS__)
#define eOP_PAR_FXYZ(OPNAME, VARNAME, name, min, max, x, y, z, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eVector3 &, eOP_PARAM_ADD_FXYZ(VARNAME, name, min, max, x, y, z), __VA_ARGS__)
#define eOP_PAR_FXYZW(OPNAME, VARNAME, name, min, max, x, y, z, w, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eVector4 &, eOP_PARAM_ADD_FXYZW(VARNAME, name, min, max, x, y, z, w), __VA_ARGS__)
#define eOP_PAR_LINK_MATERIAL(OPNAME, VARNAME, name, allowedLinks, requiredLink, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eIMaterialOp*, eOP_PARAM_ADD_LINK(const eIMaterialOp*, VARNAME, name, allowedLinks, requiredLink), __VA_ARGS__)
#define eOP_PAR_LINK_PATH(OPNAME, VARNAME, name, allowedLinks, requiredLink, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eIPathOp*, eOP_PARAM_ADD_LINK(const eIPathOp*, VARNAME, name, allowedLinks, requiredLink), __VA_ARGS__)
#define eOP_PAR_LINK_MESH(OPNAME, VARNAME, name, allowedLinks, requiredLink, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eIMeshOp*, eOP_PARAM_ADD_LINK(const eIMeshOp*, VARNAME, name, allowedLinks, requiredLink), __VA_ARGS__)
#define eOP_PAR_LINK_MODEL(OPNAME, VARNAME, name, allowedLinks, requiredLink, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eIModelOp*, eOP_PARAM_ADD_LINK(const eIModelOp*, VARNAME, name, allowedLinks, requiredLink), __VA_ARGS__)
#define eOP_PAR_LINK_BMP(OPNAME, VARNAME, name, allowedLinks, requiredLink, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eIBitmapOp*, eOP_PARAM_ADD_LINK(const eIBitmapOp*, VARNAME, name, allowedLinks, requiredLink), __VA_ARGS__)
#define eOP_PAR_LINK_POV(OPNAME, VARNAME, name, allowedLinks, requiredLink, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eIPovOp*, eOP_PARAM_ADD_LINK(const eIPovOp*, VARNAME, name, allowedLinks, requiredLink), __VA_ARGS__)
#define eOP_PAR_LINK_R2T(OPNAME, VARNAME, name, allowedLinks, requiredLink, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eIRenderToTextureOp*, eOP_PARAM_ADD_LINK(const eIRenderToTextureOp*, VARNAME, name, allowedLinks, requiredLink), __VA_ARGS__)
#define eOP_PAR_LINK_DEMO(OPNAME, VARNAME, name, allowedLinks, requiredLink, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eDemoOp*, eOP_PARAM_ADD_LINK(const eDemoOp*, VARNAME, name, allowedLinks, requiredLink), __VA_ARGS__)
#define eOP_PAR_LINK(OPNAME, VARNAME, name, allowedLinks, requiredLink, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, const eIOperator*, eOP_PARAM_ADD_LINK(const eIOperator*, VARNAME, name, allowedLinks, requiredLink), __VA_ARGS__)
#ifdef ePLAYER
#define eOP_PAR_LABEL(OPNAME, VARNAME, name, caption, ...)																			\
	eOP_PAR_GLUE1(__VA_ARGS__)
#else
#define eOP_PAR_LABEL(OPNAME, VARNAME, name, caption, ...)																			\
	eOP_PAR_GLUE_CHOOSER(OPNAME, VARNAME)(OPNAME, VARNAME, void*, eOP_PARAM_ADD_LABEL(VARNAME, name, caption), __VA_ARGS__)
#endif
#endif // OP_MACROS_HPP