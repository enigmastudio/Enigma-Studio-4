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

static const eScriptInstruction INSTRUCTIONS[OC_COUNT] =
{
    { OC_PUSHS,  "pushs",  1, ST_SCALAR, { } },
    { OC_PUSHV,  "pushv",  1, ST_VECTOR, { } },
    { OC_EPUSHS, "epushs", 1, ST_SCALAR, { } },
    { OC_EPUSHV, "epushv", 1, ST_VECTOR, { } },
    { OC_POPS,   "pops",   1, ST_NONE,   ST_SCALAR },
    { OC_POPV,   "popv",   1, ST_NONE,   ST_VECTOR },
    { OC_EPOPS,  "epops",  1, ST_NONE,   ST_SCALAR },
    { OC_EPOPV,  "epopv",  1, ST_NONE,   ST_VECTOR },
    { OC_SCALAR, "scalar", 1, ST_SCALAR, { } },

    { OC_ADDS,   "adds",   0, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_SUBS,   "subs",   0, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_MULS,   "muls",   0, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_DIVS,   "divs",   0, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_NEGS,   "negs",   0, ST_SCALAR, { ST_SCALAR } },
    { OC_ADDV,   "addv",   0, ST_VECTOR, { ST_VECTOR, ST_VECTOR } },
    { OC_SUBV,   "subv",   0, ST_VECTOR, { ST_VECTOR, ST_VECTOR } },
    { OC_MULV,   "mulv",   0, ST_VECTOR, { ST_VECTOR, ST_VECTOR } },
    { OC_DIVV,   "divv",   0, ST_VECTOR, { ST_VECTOR, ST_VECTOR } },
    { OC_NEGV,   "negv",   0, ST_VECTOR, { ST_VECTOR } },
                           
    { OC_SIN,    "sin",    0, ST_SCALAR, { ST_SCALAR } },
    { OC_COS,    "cos",    0, ST_SCALAR, { ST_SCALAR } },
    { OC_SQRT,   "sqrt",   0, ST_SCALAR, { ST_SCALAR } },
    { OC_MIN,    "min",    0, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_MAX,    "max",    0, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_MOD,    "mod",    0, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_POW,    "pow",    0, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_ABS,    "abs",    0, ST_SCALAR, { ST_SCALAR } },
    { OC_PATH,   "path",   2, ST_VECTOR, { ST_STRING, ST_SCALAR } }
};

static const eScriptIntrinsic INTRINSICS[] =
{
    { OC_SIN,  "sin",  1, ST_SCALAR, { ST_SCALAR } },
    { OC_COS,  "cos",  1, ST_SCALAR, { ST_SCALAR } },
    { OC_SQRT, "sqrt", 1, ST_SCALAR, { ST_SCALAR } },
    { OC_MIN,  "min",  2, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_MAX,  "max",  2, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_MOD,  "mod",  2, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_POW,  "pow",  2, ST_SCALAR, { ST_SCALAR, ST_SCALAR } },
    { OC_ABS,  "abs",  1, ST_SCALAR, { ST_SCALAR} },
    { OC_PATH, "path", 2, ST_VECTOR, { ST_STRING, ST_SCALAR } }
};

static const eScriptBinOp BINOPS[] =
{
    { OC_ADDS, '+' },
    { OC_SUBS, '-' },
    { OC_MULS, '*' },
    { OC_DIVS, '/' },
    { OC_ADDV, '+' },
    { OC_SUBV, '-' },
    { OC_MULV, '*' },
    { OC_DIVV, '/' },
};

static const eString TOKEN_STRINGS[TOKEN_END] =
{
    "=", "operator (+, -, *, /)", "identifier", "number",
    "(", "[", ")", "]", ";", ".", ",", "scalar", "vector"
};

static const eScriptSel SELECTORS[4] =
{
    {'x', 0}, {'y', 1}, {'z', 2}, {'w', 3}
};

// scanner

void eScriptScanner::tokenize(const eString &source)
{
    m_tokens.clear();
    m_curIndex = 0;

    eU32 line = 1;
    const char *c = source;

    while (*c != '\0')
    {
        // skip formatting
        while (*c == ' ' || *c == '\r' || *c == '\n' || *c == '\t')
        {
            if (*c++ == '\n')
                line++;
        }

        // handle one-line comments
        if (c[0] == '/' && c[1] == '/')
        {
            while (*c != '\n' && *c != '\0')
                c++;
            
            continue;
        }

        if (*c == '=') // assignment
        {
            m_tokens.append(eScriptToken(TOKEN_ASSIGN));
            c++;
        }
        else if (*c == ';') // end of statement
        {
            m_tokens.append(eScriptToken(TOKEN_SEMICOLON));
            c++;
        }
        else if (*c == ',') // intrinsic parameter separator
        {
            m_tokens.append(eScriptToken(TOKEN_COMMA));
            c++;
        }
        else if (*c == '.') // dot
        {
            m_tokens.append(eScriptToken(TOKEN_DOT));
            c++;
        }
        else if (*c == '+' || *c == '-' || *c == '*' || *c == '/') // operator
        {
            eScriptToken t(TOKEN_OPERATOR);
            t.op = *c++;
            m_tokens.append(t);
        }
        else if (*c == '"') // quotation mark
        {
            m_tokens.append(eScriptToken(TOKEN_QUOT));
            c++;
        }
        else if (*c == '(') // brace open
        {
            m_tokens.append(eScriptToken(TOKEN_OBRACE));
            c++;
        }
        else if (*c == ')') // brace close
        {
            m_tokens.append(eScriptToken(TOKEN_CBRACE));
            c++;
        }
        else if (*c == '[') // bracket open
        {
            m_tokens.append(eScriptToken(TOKEN_OBRACKET));
            c++;
        }
        else if (*c == ']') // bracket close
        {
            m_tokens.append(eScriptToken(TOKEN_CBRACKET));
            c++;
        }
        else if (eIsDigit(*c)) // number
        {
            eString num(*c++);
            while (eIsDigit(*c))
                num += *c++;

            if (*c == '.')
            {
                num += *c++;
                while (eIsDigit(*c))
                    num += *c++;
            }

            eScriptToken t(TOKEN_NUMBER);
            t.scalar = eStrToFloat(num);
            m_tokens.append(t);
        }
        else if (eIsAlpha(*c) || *c == '_') // identifier (intrinsic, variable) or declaration)
        {
            eString ident(*c++);
            while (eIsAlpha(*c) || eIsDigit(*c) || *c == '_')
                ident += *c++;

            eScriptTokenType tt = TOKEN_IDENTIFIER;
            if (ident == "scalar")
                tt = TOKEN_DECLSCALAR;
            else if (ident == "vector")
                tt = TOKEN_DECLVEC;
            
            m_tokens.append(eScriptToken(tt));
            eStrCopy(m_tokens.last().ident, ident);
        }
        else
        {
            c++;
            continue;
        }

        // set line token was found in
        m_tokens.last().line = line;
    }

    // append end indicator
    m_tokens.append(eScriptToken(TOKEN_END));
    m_tokens.last().line = line;
}

const eScriptToken & eScriptScanner::peek()
{
    eASSERT(m_curIndex < m_tokens.size());
    return m_tokens[m_curIndex];
}

void eScriptScanner::next()
{
    if ((eInt)m_curIndex < (eInt)m_tokens.size()-1)
        m_curIndex++;
}

// code generator

void eScriptCodeGen::generate(const eScriptNode *root, eScript &script)
{
    m_byteCode.clear();
    _generate(root);
    script.byteCode.append(m_byteCode);
}

void eScriptCodeGen::_generate(const eScriptNode *n)
{
    if (n->left && n->type != SNT_ASSIGN)
        _generate(n->left);
    if (n->right)
        _generate(n->right);

    switch (n->type)
    {
    case SNT_VECTOR:
    case SNT_COMBINE:
    case SNT_STRING:
        break;

    case SNT_BINOP:
        _emitByte(n->opCode);
        break;

    case SNT_SCALAR:
        _emitByte(OC_SCALAR);
        _emitFloat(n->scalar);
        break;

    case SNT_VARIABLE:
        if (n->symbol->external)
            _emitByte(n->symbol->type == ST_SCALAR || n->varSel ? OC_EPUSHS : OC_EPUSHV);
        else
            _emitByte(n->symbol->type == ST_SCALAR || n->varSel ? OC_PUSHS : OC_PUSHV);
        _emitOffset(n->symbol->offset+(n->varSel ? n->varSel->offset : 0));
        break;

    case SNT_ASSIGN:
        if (n->left->symbol->external)
            _emitByte(n->left->symbol->type == ST_SCALAR || n->left->varSel ? OC_EPOPS : OC_EPOPV);
        else
            _emitByte(n->left->symbol->type == ST_SCALAR || n->left->varSel ? OC_POPS : OC_POPV);
        _emitOffset(n->left->symbol->offset+(n->left->varSel ? n->left->varSel->offset : 0));
        break;

    case SNT_INTRINSIC:
        _emitByte(n->intrinsic->opCode);
        if (n->intrinsic->opCode == OC_PATH)
            _emitU32(n->opId);
        break;

    case SNT_NEGATE:
        _emitByte(n->varType == ST_SCALAR ? OC_NEGS : OC_NEGV);
        break;

    default:
        eASSERT(eFALSE);
        break;
    }
}

void eScriptCodeGen::_emitBytes(const void *b, eU32 count)
{
    for (eU32 i=0; i<count; i++)
        m_byteCode.append(((eU8 *)b)[i]);
}

void eScriptCodeGen::_emitByte(eU8 b)
{
    _emitBytes(&b, 1);
}

void eScriptCodeGen::_emitFloat(eF32 f32)
{
	const eU32 v = *(eU32 *)&f32;
	const eU32 sign = (v>>31)&0x01;
	const eU32 exp = (v>>23)&0xff;
	const eU32 mant = (v<<9)>>9;
	const eU32 cutMant = mant>>8;
	const eU16 cutMantSign = (eU16)((cutMant<<1)|sign);

	_emitByte(exp);
	_emitByte(eLobyte(cutMantSign));
	_emitByte(eHibyte(cutMantSign));

//	_emitBytes(&flt, 4);
}

void eScriptCodeGen::_emitOffset(eU32 offset)
{
//	eWriteToLog(eString("Emitting offset: ") + eIntToStr(offset));
	// hard alert
	if((offset < 0) || (offset > 255))
		eShowError("Emitting an offset larger than byte in ScriptVM");
	_emitBytes(&offset, 1);

//    _emitBytes(&offset, 4);
}

void eScriptCodeGen::_emitU32(eU32 offset)
{
    _emitBytes(&offset, 4);
}


// compiler

eBool eScriptCompiler::compile(const eString &source, eScript &script, const eArray<eScriptExtVar> &extVars, eArray<eU32>& extVarUsed)
{
    m_symbols.clear();
    m_astNodes.clear();
    m_astNodes.reserve(1000);
    m_scanner.tokenize(source);
    m_curVarOff = 0;
    m_errors = "";

    script.source = source;

    // add external symbols
    for (eU32 i=0, index=0; i<extVars.size(); i++)
    {
        eScriptSymbol &sym = m_symbols.push();
        eStrCopy(sym.name, extVars[i].name);
        sym.type = extVars[i].type;
        sym.offset = index;
        sym.constant = extVars[i].constant;
        sym.external = eTRUE;
		sym.isReferenced = eFALSE;
		sym.isWritten = eFALSE;

        index += (sym.type == ST_SCALAR ? 1 : 4);
    }

    // generate ASTs (one for each statement)
    eArray<eScriptNode *> asts;
    while (m_scanner.peek().type != TOKEN_END)
    {
        try
        {
            asts.append(_statement());
        }
        catch (ErrorRecoveryException) // synchronize on error
        {
        }
    }

    if (m_errors == "")
    {
        for (eU32 i=0; i<m_refOps.size(); i++)
            script.refOps.append(m_refOps[i]->getId());

        // generate byte-code
        script.byteCode.clear();
        for (eU32 i=0; i<asts.size(); i++)
            if (asts[i])
                m_codeGen.generate(asts[i], script);

        // count used non-external variables
        script.mem.clear();
        script.memSize = 0;
        for (eU32 i=0; i<m_symbols.size(); i++)
            if (!m_symbols[i].external)
                script.memSize += (m_symbols[i].type == ST_SCALAR ? 1 : 4);
    }

	// translate parameter references
	extVarUsed.clear();
    for (eU32 i=0; i<extVars.size(); i++) {
		eU32 val = 0;
		if(m_symbols[i].isReferenced)	
			val |= eSCRIPT_EXTVAR_USED;
		if(m_symbols[i].isWritten)	
			val |= eSCRIPT_EXTVAR_WRITTEN;
		extVarUsed.append(val);
	}

    script.errors = m_errors;
    return (m_errors == "");
}

eString eScriptCompiler::disassemble(const eScript &script) const
{
    if (script.byteCode.isEmpty())
        return "";

    eString buf;
    const eU8 *ip = &script.byteCode[0];
    
    while (ip <= &script.byteCode.last())
    {
        const eScriptOpCode oc = (eScriptOpCode)*ip++;
        buf += eString(INSTRUCTIONS[oc].mnemonic)+"\t";

        switch (oc)
        {
        case OC_POPS:
        case OC_POPV:
        case OC_PUSHS:
        case OC_PUSHV:
        case OC_EPOPS:
        case OC_EPOPV:
        case OC_EPUSHS:
        case OC_EPUSHV:
            buf += eString("m")+eIntToStr(*(eU32 *)ip);
            ip += 4;
            break;

        case OC_SCALAR:
            buf += eFloatToStr(*(eF32 *)ip);
            ip += 4;
            break;

        case OC_PATH:
            buf += eString("id=")+eIntToStr(*(eU32 *)ip);
            ip += 4;
            break;
        }

        buf += "\n";
    }

    return buf;
}

const eString & eScriptCompiler::getErrors() const
{
    return m_errors;
}

eScriptNode * eScriptCompiler::_statement()
{
    switch (m_scanner.peek().type)
    {
    case TOKEN_DECLSCALAR:
        _declaration(ST_SCALAR);
        return nullptr;

    case TOKEN_DECLVEC:
        _declaration(ST_VECTOR);
        return nullptr;

    default:
        return _assignment();
    }
}

void eScriptCompiler::_declaration(eScriptType symType)
{
    m_scanner.next();

    while (eTRUE)
    {
        const eString &ident = m_scanner.peek().ident;
        _match(TOKEN_IDENTIFIER);
        
        if (_findSymbol(ident))
            _error(eString("symbol '")+ident+"' already declared");
        else
            _addSymbol(ident, symType);

        if (m_scanner.peek().type != TOKEN_COMMA)
            break;

        _match(TOKEN_COMMA);
    }

    _match(TOKEN_SEMICOLON);
}

eScriptNode * eScriptCompiler::_assignment()
{
    eScriptNode *l = _expression();

    if (l->type != SNT_VARIABLE)
        _error("l-value required");
    else if (l->symbol->constant)
        _error(eString("'")+l->symbol->name+"' is a constant");

    _match(TOKEN_ASSIGN);    
    eScriptNode *r = _expression();

    if (!r)
        _error("r-value expected");
    else if (l->varType != r->varType)
        _error("type mismatch");

    _match(TOKEN_SEMICOLON);

	l->symbol->isWritten = eTRUE;
    return _createNode(SNT_ASSIGN, l, r);
}

eScriptNode * eScriptCompiler::_expression()
{
    eScriptNode *r = _term();

    while (m_scanner.peek().type == TOKEN_OPERATOR)
    {
        const char op = m_scanner.peek().op;

        if (op == '+' || op == '-')
        {
            m_scanner.next();
            r = _createNode(SNT_BINOP, r, _term());
            r->varType = r->left->varType;

            if (!r->left || !r->right)
                _error("syntax error");

            if (r->left->varType == ST_SCALAR && r->right->varType == ST_SCALAR)
                r->opCode = (op == '+' ? OC_ADDS : OC_SUBS);
            else if (r->left->varType == ST_VECTOR && r->right->varType == ST_VECTOR)
                r->opCode = (op == '+' ? OC_ADDV : OC_SUBV);
            else
                _error("type mismatch");
        }
        else break;
    }

    return r;
}

eScriptNode * eScriptCompiler::_term()
{
    eScriptNode *r = _factor();
    if (!r)
        _error("syntax error");

    while (m_scanner.peek().type == TOKEN_OPERATOR)
    {
        const char op = m_scanner.peek().op;

        if (op == '*' || op == '/')
        {
            m_scanner.next();
            r = _createNode(SNT_BINOP, r, _factor());
            r->varType = r->left->varType;

            if (!r->left || !r->right)
                _error("syntax error");

            if (r->left->varType == ST_SCALAR && r->right->varType == ST_SCALAR)
                r->opCode = (op == '*' ? OC_MULS : OC_DIVS);
            else if (r->left->varType == ST_VECTOR && r->right->varType == ST_VECTOR)
                r->opCode = (op == '*' ? OC_MULV : OC_DIVV);
            else
                _error("type mismatch");
        }
        else break;
    }

    return r;
}

eScriptNode * eScriptCompiler::_factor()
{
    if (m_scanner.peek().type == TOKEN_OBRACE) // braced expression
    {
        m_scanner.next();
        eScriptNode *r = _expression();
        _match(TOKEN_CBRACE);
        return r;
    }
    else if (m_scanner.peek().type == TOKEN_OBRACKET) // vector
    {
        m_scanner.next();
        eScriptNode *n0 = _expression();
        _match(TOKEN_COMMA);
        eScriptNode *n1 = _expression();
        _match(TOKEN_COMMA);
        eScriptNode *n2 = _expression();
        _match(TOKEN_COMMA);
        eScriptNode *n3 = _expression();
        _match(TOKEN_CBRACKET);

        eScriptNode *a = _createNode(SNT_COMBINE, n0, n1);
        eScriptNode *b = _createNode(SNT_COMBINE, n2, n3);
        eScriptNode *r = _createNode(SNT_VECTOR, a, b);
        r->varType = ST_VECTOR;
        return r;
    }
    else if (m_scanner.peek().type == TOKEN_QUOT) // string
    {
        _match(TOKEN_QUOT);
        eScriptNode *r = _createNode(SNT_STRING, nullptr, nullptr);
        eStrCopy(r->str, m_scanner.peek().ident);
        r->varType = ST_STRING;
        _match(TOKEN_IDENTIFIER);
        _match(TOKEN_QUOT);
        return r;
    }
    else if (m_scanner.peek().type == TOKEN_OPERATOR && m_scanner.peek().op == '-') // negation
    {
        m_scanner.next();
        eScriptNode *r = _createNode(SNT_NEGATE, _factor(), nullptr);
        if (!r->left)
            _error("expression expected");

        r->varType = r->left->varType;
        return r;
    }
    else if (m_scanner.peek().type == TOKEN_NUMBER)
        return _number();
    else if (m_scanner.peek().type == TOKEN_IDENTIFIER)
        return _identifier();
    else
        return nullptr;
}

eScriptNode * eScriptCompiler::_identifier()
{
    eString ident = m_scanner.peek().ident;
    m_scanner.next();

    // intrinsic call
    if (m_scanner.peek().type == TOKEN_OBRACE)
    {
        return _intrinsic(ident);
    }
    else // variable
    {
        eScriptSymbol *sym = _findSymbol(ident);
        if (!sym)
            _error(eString("undefined symbol '")+ident+"'");
		// mark usage of symbol
		sym->isReferenced = eTRUE;

        eScriptNode *n = _createNode(SNT_VARIABLE, nullptr, nullptr);
        n->symbol = sym;
        n->varSel = nullptr;
        n->varType = sym->type;

        if (m_scanner.peek().type == TOKEN_DOT)
        {
            if (sym->type == ST_SCALAR)
                _error("swizzling only allowed on array types");

            m_scanner.next();
            ident = m_scanner.peek().ident;
            _match(TOKEN_IDENTIFIER);
            n->varType = ST_SCALAR;

            for (eU32 i=0; i<eELEMENT_COUNT(SELECTORS); i++)
                if (ident == SELECTORS[i].name)
                    n->varSel = &SELECTORS[i];

            if (!n->varSel)
                _error("only 'x', 'y', 'z', 'w' allowed as swizzle");
        }

        return n;
    }
}

eScriptNode * eScriptCompiler::_intrinsic(const eString &ident)
{
    // is identifier known intrinsic?
    const eScriptIntrinsic *intr = nullptr;
    for (eU32 i=0; i<eELEMENT_COUNT(INTRINSICS); i++)
        if (ident == INTRINSICS[i].name)
            intr = &INTRINSICS[i];

    // parse parameters
    eArray<eScriptNode *> params;

    _match(TOKEN_OBRACE);
    while (m_scanner.peek().type != TOKEN_CBRACE && m_scanner.peek().type != TOKEN_END)
    {
        params.append(_expression());
        if (!params.last())
            _error("no parameters provided");

        if (m_scanner.peek().type == TOKEN_COMMA)
            m_scanner.next();
    }
    _match(TOKEN_CBRACE);

    // check for errors
    if (!intr)
        _error(eString("intrinsic '")+ident+"' is unknown");
    else if (params.size() > intr->numParams)
        _error(eString("too many parameters for '")+ident+"'");
    else if (params.size() < intr->numParams)
        _error(eString("not enough parameters for '")+ident+"'");
    else
    {
        for (eU32 i=0; i<intr->numParams; i++)
            if (params[i]->varType != intr->inType[i])
                _error("wrong parameter type");

        // create AST node
        eScriptNode *l = (params.size() > 0 ? params[0] : nullptr);
        eScriptNode *r = (params.size() > 1 ? params[1] : nullptr);
        eScriptNode *n = _createNode(SNT_INTRINSIC, l, r);
        n->varType = intr->outType;
        n->intrinsic = intr;

        // path intrinsic
        if (intr->opCode == OC_PATH)
        {
            eIOperator *pathOp = nullptr;

            for (eU32 i=0; i<eDemoData::getPageCount(); i++)
            {
                eOperatorPage *opPage = eDemoData::getPageByIndex(i);
                for (eU32 j=0; j<opPage->getOperatorCount(); j++)
                {
                    eIOperator *op = eDemoData::getPageByIndex(i)->getOperatorByIndex(j);
                    if (op->getUserName() == params[0]->str)
                        pathOp = op;
                }
            }

            if (!pathOp)
                _error("path operator not found");
            else if (pathOp->getResultClass() != eOC_PATH)
                _error("path operator has wrong type");

            n->opId = pathOp->getId();
            m_refOps.append(pathOp);
        }

        return n;
    }

    return nullptr;
}

eScriptNode * eScriptCompiler::_number()
{
    eScriptNode *n = _createNode(SNT_SCALAR, nullptr, nullptr);
    n->scalar = m_scanner.peek().scalar;
    n->varType = ST_SCALAR;
    m_scanner.next();
    return n;
}

eScriptNode * eScriptCompiler::_createNode(eScriptNodeType type, eScriptNode *left, eScriptNode *right)
{
    eScriptNode *n = &m_astNodes.push();
    
    if (left) left->parent = n;
    if (right) right->parent = n;

    n->type = type;
    n->left = left;
    n->right = right;
    n->parent = nullptr;
    return n;
}

void eScriptCompiler::_addSymbol(const eString &name, eScriptType symType)
{
    eScriptSymbol &sym = m_symbols.push();
    eStrCopy(sym.name, name);
    sym.type = symType;
    sym.constant = eFALSE;
    sym.external = eFALSE;
    sym.offset = m_curVarOff;

    m_curVarOff += (symType == ST_SCALAR ? 1 : 4);
}

eScriptSymbol * eScriptCompiler::_findSymbol(const eString &name)
{
    for (eU32 i=0; i<m_symbols.size(); i++)
        if (name == m_symbols[i].name)
            return &m_symbols[i];

    return nullptr;
}

eBool eScriptCompiler::_match(eScriptTokenType tokenType)
{
    const eBool matched = (m_scanner.peek().type == tokenType);
    
    if (!matched)
        _error(eString("expected ")+TOKEN_STRINGS[tokenType]);
    else
        m_scanner.next();

    return matched;
}

void eScriptCompiler::_error(const eString &error)
{
    m_errors += (m_errors == "" ? "" : "\n"); // new line if there's already an error
    m_errors += eIntToStr(m_scanner.peek().line);
    m_errors += ": ";
    m_errors += error;

    // panic recovery: synchronize with {';'}
    while (eTRUE)
    {
        const eScriptTokenType tt = m_scanner.peek().type;

        if (tt == TOKEN_SEMICOLON)
        {
            m_scanner.next();
            break;
        }
        else if (tt == TOKEN_END)
            break;
        else
            m_scanner.next();
    }

    throw ErrorRecoveryException();
}

#endif

void eScriptVm::execute(eScript &script, eF32 *extVars)
{
    if (script.byteCode.isEmpty())
        return;

    script.mem.resize(script.memSize);
    eMemSet(m_stack, 0, sizeof(m_stack));
    m_stackPos = -1;
    m_ip = &script.byteCode[0];

    eVector4 v0, v1;

    while (m_ip <= &script.byteCode.last())
    {
        const eScriptOpCode oc = (eScriptOpCode)_readByte();

        switch (oc)
        {
        case OC_PUSHS:
            _pushs(script.mem[_readOffset()]);
            break;

        case OC_PUSHV:
            _push(&script.mem[_readOffset()], 4);
            break;

        case OC_EPUSHS:
            _pushs(extVars[_readOffset()]);
            break;

        case OC_EPUSHV:
            _push(&extVars[_readOffset()], 4);
            break;

        case OC_POPS:
            script.mem[_readOffset()] = _pops();
            break;

        case OC_POPV:
            _pop(&script.mem[_readOffset()], 4);
            break;
            
        case OC_EPOPS:
            extVars[_readOffset()] = _pops();
            break;

        case OC_EPOPV:
            _pop(&extVars[_readOffset()], 4);
            break;

        case OC_SCALAR:
            _pushs(_readFloat());
            break;

        case OC_ADDS:
            _pushs(_pops()+_pops());
            break;

        case OC_SUBS:
            m_stack[--m_stackPos] -= m_stack[m_stackPos+1];
            break;

        case OC_MULS:
            _pushs(_pops()*_pops());
            break;

        case OC_DIVS:
            m_stack[--m_stackPos] /= m_stack[m_stackPos+1];
            break;

        case OC_NEGS:
            m_stack[m_stackPos] *= -1.0f;
            break;

        case OC_ADDV:
            _pushv(_popv()+_popv());
            break;

        case OC_SUBV:
            v0 = _popv();
            v1 = _popv();
            _pushv(v0-v1);
            break;

        case OC_MULV:
            _pushv(_popv()*_popv());
            break;

        case OC_DIVV:
            v0 = _popv();
            v1 = _popv();
            v0.scale(1.0f/v1);
            _pushv(v0);
            break;

        case OC_NEGV:
            v0 = _popv();
            v0 *= -1.0f;
            _pushv(v0);
            break;

        case OC_SIN:
            m_stack[m_stackPos] = eSin(m_stack[m_stackPos]);
            break;

        case OC_COS:
            m_stack[m_stackPos] = eCos(m_stack[m_stackPos]);
            break;

        case OC_SQRT:
            m_stack[m_stackPos] = eSqrt(m_stack[m_stackPos]);
            break;

        case OC_MIN:
            _pushs(eMin(_pops(), _pops()));
            break;

        case OC_MAX:
            _pushs(eMax(_pops(), _pops()));
            break;

        case OC_MOD:
            _pushs(eMod(_pops(), _pops()));
            break;

        case OC_POW:
            _pushs(ePow(_pops(), _pops()));
            break;

        case OC_ABS:
            m_stack[m_stackPos] = eAbs(m_stack[m_stackPos]);
            break;

        case OC_PATH:
            eIOperator *op = eDemoData::findOperator(_readU32());
            if (op)
            {
                op->process(0.0f);
                _pushv(((eIPathOp *)op)->getResult().path.evaluate(_pops()));
            }
            break;
        }
    }
}

eU8 eScriptVm::_readByte()
{
    return *m_ip++;
}

eF32 eScriptVm::_readFloat()
{
    eU8	     exp = _readByte();
	const eU8 lo = _readByte();
	const eU8 hi = _readByte();
	const eU32 cutMantSign = eMakeWord(lo, hi);
	const eU32 cutMant = cutMantSign>>1;
	const eU32 mant = cutMant<<8;
	const eU32 sign = cutMantSign&0x00000001;
	const eU32 res = (sign<<31)|(exp<<23)|mant;
	return *(eF32 *)&res;

/*
    const eF32 flt = *(eF32 *)m_ip;
    m_ip += sizeof(eF32);
    return flt;
*/
}

eVector4 eScriptVm::_readVector()
{
    const eVector4 v = *(eVector4 *)m_ip;
    m_ip += sizeof(eVector4);
    return v;
}

eU32 eScriptVm::_readU32()
{
    const eU32 off = *(eU32 *)m_ip;
    m_ip += sizeof(eU32);
    return off;
}

eU32 eScriptVm::_readOffset()
{
	m_ip++;
	return *(m_ip-1);
}

void eScriptVm::_push(const eF32 *data, eU32 count)
{
    eASSERT(m_stackPos+count < STACK_SIZE-1);

    for (eU32 i=0; i<count; i++)
        m_stack[++m_stackPos] = data[i];
}

void eScriptVm::_pushs(eF32 flt)
{
    _push(&flt, 1);
}

void eScriptVm::_pushv(const eVector4 &v)
{
    _push(&v.x, 4);
}

void eScriptVm::_pop(eF32 *data, eU32 count)
{
    eASSERT(m_stackPos-count >= 0);
    
    for (eInt i=(eInt)count-1; i>=0; i--)
        data[i] = m_stack[m_stackPos--];
}

eF32 eScriptVm::_pops()
{
    eF32 s;
    _pop(&s, 1);
    return s;
}

eVector4 eScriptVm::_popv()
{
    eVector4 v;
    _pop(&v.x, 4);
    return v;
}












void updateWriteMasks(eScript &script)
{
    eArray<eU32> mm;

    if (!script.byteCode.isEmpty())
    {
        const eU8 *ip = &script.byteCode[0];
        eU32 off;
    
        while (ip <= &script.byteCode.last())
        {
            switch ((eScriptOpCode)*ip++)
            {
            case OC_EPUSHS:
                off = *(eU32 *)ip;
                mm.appendUnique(off);
                ip += 4;
                break;

            case OC_EPUSHV:
                off = *(eU32 *)ip;
                mm.appendUnique(off);
                mm.appendUnique(off+1);
                mm.appendUnique(off+2);
                mm.appendUnique(off+3);
                ip += 4;
                break;

            case OC_SCALAR:
                ip += 3; // is this correct? floats now only 3 bytes?
                break;

            default:
                ip += 4;
                break;
            }
        }
    }

    script.writeMasks.resize(20*4);
    eMemSet(&script.writeMasks[0], eFALSE, 20*4*sizeof(eU32));

    for (eU32 i=0; i<mm.size(); i++)
        script.writeMasks[mm[i]] = eTRUE;
}