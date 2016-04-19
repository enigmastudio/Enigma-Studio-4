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

#ifndef SCRIPT_HPP
#define SCRIPT_HPP

#define eSCRIPT_EXTVAR_USED 1
#define eSCRIPT_EXTVAR_WRITTEN 2

// redeclare operator array because script needs
// to be included before operator classes
class eIOperator;
typedef eArray<eIOperator *> eIOpPtrArray;

struct eScript
{
    eByteArray          byteCode;
    eU32                memSize;
    eArray<eF32>        mem;
    eArray<eID>         refOps;
    eArray<eBool>       writeMasks; // write mask for each external variable

#ifdef eEDITOR
    eString             source;
    eString             errors;
#endif
};

enum eScriptOpCode
{
    OC_PUSHS,
    OC_PUSHV,
    OC_EPUSHS,
    OC_EPUSHV,
    OC_POPS,
    OC_POPV,
    OC_EPOPS,
    OC_EPOPV,
    OC_SCALAR,

    OC_ADDS,
    OC_SUBS,
    OC_MULS,
    OC_DIVS,
    OC_NEGS,
    OC_ADDV,
    OC_SUBV,
    OC_MULV,
    OC_DIVV,
    OC_NEGV,

    OC_SIN,
    OC_COS,
    OC_SQRT,
    OC_MIN,
    OC_MAX,
    OC_MOD,
    OC_POW,
    OC_ABS,
    OC_PATH,

    OC_COUNT
};

#ifdef eEDITOR

enum eScriptTokenType
{
    TOKEN_ASSIGN,
    TOKEN_OPERATOR,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_OBRACE,
    TOKEN_OBRACKET,
    TOKEN_CBRACE,
    TOKEN_CBRACKET,
    TOKEN_SEMICOLON,
    TOKEN_QUOT,
    TOKEN_DOT,
    TOKEN_COMMA,
    TOKEN_DECLSCALAR,
    TOKEN_DECLVEC,
    TOKEN_END
};

struct eScriptToken
{
    eScriptToken(eScriptTokenType tt) : type(tt)
    {
    }

    eScriptTokenType    type;
    eChar               op;
    eF32                scalar;
    eChar               ident[64];
    eU32                line;
};

enum eScriptType
{
    ST_NONE,
    ST_SCALAR,
    ST_VECTOR,
    ST_STRING,
};

struct eScriptInstruction
{
    eScriptOpCode       opCode;
    eChar               mnemonic[64];
    eU32                numOperands;
    eScriptType         outType;
    eScriptType         inType[4];
};

struct eScriptBinOp
{
    eScriptOpCode       opCode;
    eChar               op;
};

struct eScriptIntrinsic
{
    eScriptOpCode       opCode;
    eChar               name[64];
    eU32                numParams;
    eScriptType         outType;
    eScriptType         inType[4];
};

struct eScriptSymbol
{
    eChar               name[64];
    eScriptType         type;
    eBool               external;
    eBool               constant;
    eU32                offset;
	eBool				isReferenced;
	eBool				isWritten;
};

struct eScriptExtVar
{
    eChar               name[64];
    eBool               constant;
    eScriptType         type;
};

enum eScriptNodeType
{
    SNT_ASSIGN,
    SNT_BINOP,
    SNT_INTRINSIC,
    SNT_SCALAR,
    SNT_VECTOR,
    SNT_STRING,
    SNT_COMBINE,
    SNT_VARIABLE,
    SNT_NEGATE,
};

struct eScriptSel
{
    eChar               name;
    eU32                offset;
};

struct eScriptNode
{
    eScriptNodeType     type;
    eScriptNode *       parent;
    eScriptNode *       left;
    eScriptNode *       right;
    eScriptSymbol *     symbol;
    eF32                scalar;
    eChar               str[64];
    eID                 opId;
    const eScriptSel *  varSel;
    eScriptType         varType;
    eScriptOpCode       opCode;
    const eScriptIntrinsic *  intrinsic;
};

class eScriptScanner
{
public:
    void                tokenize(const eString &source);
   const eScriptToken & peek();
    void                next();

private:
    typedef eArray<eScriptToken> eScriptTokenVector;

private:
    eScriptTokenVector  m_tokens;
    eU32                m_curIndex;
};

class eScriptCodeGen
{
public:
    void                generate(const eScriptNode *root, eScript &script);

private:
    void                _generate(const eScriptNode *n);
    void                _emitBytes(const void *b, eU32 count);
    void                _emitByte(eU8 b);
    void                _emitFloat(eF32 flt);
    void                _emitOffset(eU32 offset);
    void                _emitU32(eU32 offset);

private:
    eByteArray          m_byteCode;
};

class eScriptCompiler
{
public:
    eBool               compile(const eString &source, eScript &script, const eArray<eScriptExtVar> &extVars, eArray<eU32>& extVarUsed);
    eString             disassemble(const eScript &script) const;
    const eString &     getErrors() const;

private:
    eScriptNode *       _statement();
    void                _declaration(eScriptType symType);
    eScriptNode *       _assignment();
    eScriptNode *       _expression();
    eScriptNode *       _term();
    eScriptNode *       _factor();
    eScriptNode *       _identifier();
    eScriptNode *       _intrinsic(const eString &ident);
    eScriptNode *       _number();

    eScriptNode *       _createNode(eScriptNodeType type, eScriptNode *left, eScriptNode *right);
    void                _addSymbol(const eString &name, eScriptType symType);
    eScriptSymbol *     _findSymbol(const eString &name);
    eBool               _match(eScriptTokenType tokenType);
    void                _error(const eString &error);

private:
    class ErrorRecoveryException
    {
    };

private:
    typedef eArray<eScriptNode> eScriptNodeArray;
    typedef eArray<eScriptSymbol> eScriptSymbolArray;

private:
    eScriptScanner      m_scanner;
    eScriptCodeGen      m_codeGen;
    eScriptNodeArray    m_astNodes;
    eScriptSymbolArray  m_symbols;
    eIOpPtrArray        m_refOps;
    eString             m_errors;
    eU32                m_curVarOff;
};

#endif

class eScriptVm
{
public:
    void                execute(eScript &script, eF32 *extVars);

private:
    eU8                 _readByte();
    eF32                _readFloat();
    eVector4            _readVector();
    eU32                _readU32();
    eU32                _readOffset();

    void                _push(const eF32 *data, eU32 count);
    void                _pushs(eF32 flt);
    void                _pushv(const eVector4 &v);

    void                _pop(eF32 *data, eU32 count);
    eF32                _pops();
    eVector4            _popv();

private:
    static const eInt   STACK_SIZE = 128;

private:
    const eU8 *         m_ip;
    eF32                m_stack[STACK_SIZE];
    eInt                m_stackPos;
};

#endif