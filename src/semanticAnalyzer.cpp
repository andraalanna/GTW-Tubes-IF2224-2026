#include "semanticAnalyzer.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>


SemanticAnalyzer::SemanticAnalyzer(SymbolTable &symbolTable) : st(symbolTable) {}


void SemanticAnalyzer::analyze(ASTNodePtr root)
{
    if (!root)
    {
        semanticError("AST is empty, there is nothing to be analyzed.");
        return;
    }

    ProgramNode *prog = dynamic_cast<ProgramNode *>(root.get());
    if (!prog)
    {
        semanticError("AST root is not a ProgramNode.");
        return;
    }

    visitProgram(prog);
}



void SemanticAnalyzer::visitProgram(ProgramNode *node)
{

    int idx = st.enterTab(
        node->name,
        AllowedObj::PROGRAM,
        DataType::VOID,
        0,  
        1,
        0,
        0
    );

    node->tabIndex = idx;
    node->lexLevel = st.currentLevel; // = 0
    node->dtype    = st.tab[idx].type;

    int bodyBlock = st.pushScope();
    st.tab[idx].ref = bodyBlock;

    for (auto &decl : node->declarations)
    {
        if (decl) visitStatement(decl.get());
    }
    
    // int bodyBlock = st.pushScope();
    // st.tab[idx].ref = bodyBlock; 
    
    if (node->body)
        visitCompoundStmt(dynamic_cast<CompoundStmtNode *>(node->body.get()));

    st.popScope();
}


void SemanticAnalyzer::visitVarDecl(VarDeclNode *node)
{
    if (st.lookupCurrentScope(node->varName) != -1)
    {
        redeclarationError(node->varName);
        node->dtype = DataType::UNKNOWN;
        return;
    }

    int      typeRef   = node->typeRef;
    DataType finalType = node->varType;

    int offset = st.btab[st.currentBlock].vsze;

    int sz = elementSize(finalType, typeRef);
    st.btab[st.currentBlock].vsze += sz;

    int idx = st.enterTab(
        node->varName,
        AllowedObj::VARIABLE,
        finalType,
        typeRef,
        1,                
        st.currentLevel,
        offset
    );

    node->tabIndex = idx;
    node->lexLevel = st.currentLevel;
    node->dtype    = finalType;
}


void SemanticAnalyzer::visitConstDecl(ConstDeclNode *node)
{
    if (st.lookupCurrentScope(node->constName) != -1)
    {
        redeclarationError(node->constName);
        node->dtype = DataType::UNKNOWN;
        return;
    }

    DataType t = node->constType;

    int adrValue = 0;
    if (t == DataType::INTEGER || t == DataType::BOOLEAN)
    {
        try   { adrValue = std::stoi(node->value); }
        catch (...) { adrValue = 0; }
    }

    int idx = st.enterTab(
        node->constName,
        AllowedObj::CONSTANT,
        t,
        0,               
        1,               
        st.currentLevel,
        adrValue
    );

    node->tabIndex = idx;
    node->lexLevel = st.currentLevel;
    node->dtype    = t;
}



void SemanticAnalyzer::visitTypeDecl(TypeDeclNode *node)
{
    if (st.lookupCurrentScope(node->typeName) != -1)
    {
        redeclarationError(node->typeName);
        node->dtype = DataType::UNKNOWN;
        return;
    }

    int ref = node->typeRef;

    // Validasi index type untuk ARRAY
    if (node->baseType == DataType::ARRAY &&
        ref >= 0 && ref < (int)st.atab.size())
    {
        if (!isValidIndexType(st.atab[ref].xtyp))
            invalidIndexTypeError(st.atab[ref].xtyp);
    }

    int idx = st.enterTab(
        node->typeName,
        AllowedObj::TYPE,
        node->baseType,
        ref,
        1,
        st.currentLevel,
        0
    );

    node->tabIndex = idx;
    node->lexLevel = st.currentLevel;
    node->dtype    = node->baseType;
}

void SemanticAnalyzer::visitProcDecl(ProcDeclNode *node)
{
    if (st.lookupCurrentScope(node->procName) != -1)
    {
        redeclarationError(node->procName);
        return;
    }

    int declLevel = st.currentLevel;

    int procIdx = st.enterTab(
        node->procName,
        AllowedObj::PROCEDURE,
        DataType::VOID,
        0,           // ref diisi setelah pushScope
        1,
        declLevel,
        0
    );

    int newBlock = st.pushScope();
    st.tab[procIdx].ref = newBlock;

    for (auto &param : node->params)
    {
        if (!param) continue;

        VarDeclNode *p = dynamic_cast<VarDeclNode *>(param.get());
        if (!p) continue;

        if (st.lookupCurrentScope(p->varName) != -1)
        {
            redeclarationError(p->varName);
            continue;
        }

        int nrm    = (p->typeRef < 0) ? 0 : 1;
        int tref   = (p->typeRef < 0) ? 0 : p->typeRef;

        int offset = st.btab[newBlock].psze;
        st.btab[newBlock].psze += elementSize(p->varType, tref);

        int pIdx = st.enterTab(
            p->varName,
            AllowedObj::VARIABLE,
            p->varType,
            tref,
            nrm,
            st.currentLevel,
            offset
        );

        p->tabIndex = pIdx;
        p->lexLevel = st.currentLevel;
        p->dtype    = p->varType;
    }
    st.btab[newBlock].lpar = st.btab[newBlock].last;

    node->tabIndex = procIdx;
    node->lexLevel = declLevel;
    node->dtype    = DataType::VOID;

    if (node->body)
        visitCompoundStmt(dynamic_cast<CompoundStmtNode *>(node->body.get()));

    st.popScope();
}


void SemanticAnalyzer::visitFuncDecl(FuncDeclNode *node)
{
    if (st.lookupCurrentScope(node->funcName) != -1)
    {
        redeclarationError(node->funcName);
        return;
    }

    int declLevel = st.currentLevel;

    int funcIdx = st.enterTab(
        node->funcName,
        AllowedObj::FUNCTION,
        node->returnType,
        0,           // ref diisi setelah pushScope
        1,
        declLevel,
        0
    );

    int newBlock = st.pushScope();
    st.tab[funcIdx].ref = newBlock;

    for (auto &param : node->params)
    {
        if (!param) continue;

        VarDeclNode *p = dynamic_cast<VarDeclNode *>(param.get());
        if (!p) continue;

        if (st.lookupCurrentScope(p->varName) != -1)
        {
            redeclarationError(p->varName);
            continue;
        }

        int nrm  = (p->typeRef < 0) ? 0 : 1;
        int tref = (p->typeRef < 0) ? 0 : p->typeRef;

        int offset = st.btab[newBlock].psze;
        // FIX: teruskan tref agar ARRAY/RECORD punya ukuran yang benar
        st.btab[newBlock].psze += elementSize(p->varType, tref);

        int pIdx = st.enterTab(
            p->varName,
            AllowedObj::VARIABLE,
            p->varType,
            tref,
            nrm,
            st.currentLevel,
            offset
        );

        p->tabIndex = pIdx;
        p->lexLevel = st.currentLevel;
        p->dtype    = p->varType;
    }

    st.btab[newBlock].lpar = st.btab[newBlock].last;

    // FIX: gunakan declLevel, bukan currentLevel - 1
    node->tabIndex = funcIdx;
    node->lexLevel = declLevel;
    node->dtype    = node->returnType;

    if (node->body)
        visitCompoundStmt(dynamic_cast<CompoundStmtNode *>(node->body.get()));

    st.popScope();
}


void SemanticAnalyzer::visitCompoundStmt(CompoundStmtNode *node)
{
    if (!node) return;
    for (auto &stmt : node->statements)
    {
        if (stmt) visitStatement(stmt.get());
    }
}

// =============================================================
// visitStatement — dispatcher utama
//
// Deklarasi (Orang 4) ditangani di sini.
// Statement & ekspresi (Orang 5) ditambahkan di blok bawah.
// =============================================================

void SemanticAnalyzer::visitStatement(ASTNode *node)
{
    if (!node) return;

    // --- Deklarasi (Orang 4) ---
    if (auto *n = dynamic_cast<VarDeclNode *>(node))   { visitVarDecl(n);   return; }
    if (auto *n = dynamic_cast<ConstDeclNode *>(node)) { visitConstDecl(n); return; }
    if (auto *n = dynamic_cast<TypeDeclNode *>(node))  { visitTypeDecl(n);  return; }
    if (auto *n = dynamic_cast<ProcDeclNode *>(node))  { visitProcDecl(n);  return; }
    if (auto *n = dynamic_cast<FuncDeclNode *>(node))  { visitFuncDecl(n);  return; }

    // --- Statement & Ekspresi (Orang 5) ---
    if (auto *n = dynamic_cast<AssignNode *>(node))   { visitAssign(n);   return; }
    if (auto *n = dynamic_cast<IfNode *>(node))       { visitIf(n);       return; }
    if (auto *n = dynamic_cast<WhileNode *>(node))    { visitWhile(n);    return; }
    if (auto *n = dynamic_cast<ForNode *>(node))      { visitFor(n);      return; }
    if (auto *n = dynamic_cast<RepeatNode *>(node))   { visitRepeat(n);   return; }
    if (auto *n = dynamic_cast<CaseNode *>(node))     { visitCase(n);     return; }
    if (auto *n = dynamic_cast<ProcCallNode *>(node)) { visitProcCall(n); return; }
    if (auto *n = dynamic_cast<CompoundStmtNode *>(node)) { visitCompoundStmt(n); return; }

    // --- Expressions ---
    if (auto *n = dynamic_cast<VarNode *>(node))      { visitVar(n);      return; }
    if (auto *n = dynamic_cast<BinOpNode *>(node))    { visitBinOp(n);    return; }

    if (auto *n = dynamic_cast<ArrayAccessNode *>(node)) { visitArrayAccess(n); return; }
    if (auto *n = dynamic_cast<NumberNode *>(node))      { /* dtype sudah di-set saat build */ return; }
    if (auto *n = dynamic_cast<StringNode *>(node))      { /* dtype sudah di-set saat build */ return; }
    if (auto *n = dynamic_cast<CharNode *>(node))        { /* dtype sudah di-set saat build */ return; }
    if (auto *n = dynamic_cast<UnaryOpNode *>(node))     { visitUnaryOp(n); return; }
}

DataType SemanticAnalyzer::resolveTypeName(const std::string &typeName, int &outRef)
{
    outRef = 0;

    int idx = st.lookup(typeName);
    if (idx == -1)
    {
        undeclaredError(typeName);
        return DataType::UNKNOWN;
    }

    if (st.tab[idx].obj != AllowedObj::TYPE)
    {
        wrongObjectKindError(typeName, AllowedObj::TYPE, st.tab[idx].obj);
        return DataType::UNKNOWN;
    }

    outRef = st.tab[idx].ref;
    return st.tab[idx].type;
}


int SemanticAnalyzer::elementSize(DataType t, int ref) const
{
    switch (t)
    {
    case DataType::INTEGER: return 1;
    case DataType::REAL:    return 2;
    case DataType::CHAR:    return 1;
    case DataType::BOOLEAN: return 1;
    case DataType::STRING:  return 4;

    case DataType::ARRAY:
        // Ukuran total array sudah dihitung saat enterAtab: (high-low+1)*elsz
        if (ref >= 0 && ref < (int)st.atab.size())
            return st.atab[ref].size;
        return 0; // fallback: atab belum tersedia (array anonim belum diproses)

    case DataType::RECORD:
        // Ukuran record = total variabel di dalam blok record tersebut
        if (ref >= 0 && ref < (int)st.btab.size())
            return st.btab[ref].vsze;
        return 0; // fallback: btab belum tersedia

    default:
        return 1;
    }
}

void SemanticAnalyzer::visitAssign(AssignNode *node)
{
    if (node->target) visitStatement(node->target.get());
    if (node->value) visitStatement(node->value.get());

    if (node->target && node->value)
    {
        // Skip check kalau value adalah VOID (procedure call bukan function)
        if (node->value->dtype != DataType::VOID)
        {
            if (!isAssignCompatible(node->target->dtype, node->value->dtype))
            {
                assignIncompatibleError(node->target->dtype, node->value->dtype);
            }
        }
    }
    node->dtype = DataType::VOID;
}
void SemanticAnalyzer::visitBinOp(BinOpNode *node)
{
    if (node->left) visitStatement(node->left.get());
    if (node->right) visitStatement(node->right.get());

    if (node->left && node->right)
    {
        DataType inferredType = inferBinOpType(node->op, node->left->dtype, node->right->dtype);
        if (inferredType == DataType::UNKNOWN)
        {
            if (node->left->dtype != DataType::UNKNOWN && node->right->dtype != DataType::UNKNOWN)
            {
                invalidOperandError(node->op, node->left->dtype, node->right->dtype);
            }
        }
        node->dtype = inferredType;
    }
    else
    {
        node->dtype = DataType::UNKNOWN;
    }
}

void SemanticAnalyzer::visitVar(VarNode *node)
{
    int idx = st.lookup(node->varName);
    if (idx == -1)
    {
        undeclaredError(node->varName);
        node->dtype = DataType::UNKNOWN;
        return;
    }

    AllowedObj obj = st.tab[idx].obj;
    if (obj != AllowedObj::VARIABLE && obj != AllowedObj::CONSTANT && obj != AllowedObj::FUNCTION)
    {
        wrongObjectKindError(node->varName, AllowedObj::VARIABLE, obj);
        node->dtype = DataType::UNKNOWN;
        return;
    }

    node->tabIndex = idx;
    node->lexLevel = st.tab[idx].lev;
    node->dtype = st.tab[idx].type;
}

void SemanticAnalyzer::visitIf(IfNode *node)
{
    if (node->condition)
    {
        visitStatement(node->condition.get());
        if (node->condition->dtype != DataType::BOOLEAN && node->condition->dtype != DataType::UNKNOWN)
        {
            nonBooleanConditionError("if", node->condition->dtype);
        }
    }

    if (node->thenBranch) visitStatement(node->thenBranch.get());
    if (node->elseBranch) visitStatement(node->elseBranch.get());

    node->dtype = DataType::VOID;
}

void SemanticAnalyzer::visitWhile(WhileNode *node)
{
    if (node->condition)
    {
        visitStatement(node->condition.get());
        if (node->condition->dtype != DataType::BOOLEAN && node->condition->dtype != DataType::UNKNOWN)
        {
            nonBooleanConditionError("while", node->condition->dtype);
        }
    }

    if (node->body) visitStatement(node->body.get());

    node->dtype = DataType::VOID;
}

void SemanticAnalyzer::visitFor(ForNode *node)
{
    int idx = st.lookup(node->varName);
    DataType controlType = DataType::UNKNOWN;
    
    if (idx == -1) undeclaredError(node->varName);
    else
    {
        if (st.tab[idx].obj != AllowedObj::VARIABLE)
        {
            wrongObjectKindError(node->varName, AllowedObj::VARIABLE, st.tab[idx].obj);
        }
        else
        {
            controlType = st.tab[idx].type;
        }
    }

    if (node->fromExpr) 
    {
        visitStatement(node->fromExpr.get());
        if (controlType != DataType::UNKNOWN && node->fromExpr->dtype != DataType::UNKNOWN)
        {
            if (!isAssignCompatible(controlType, node->fromExpr->dtype))
            {
                assignIncompatibleError(controlType, node->fromExpr->dtype, node->varName);
            }
        }
    }

    if (node->toExpr)
    {
        visitStatement(node->toExpr.get());
        if (controlType != DataType::UNKNOWN && node->toExpr->dtype != DataType::UNKNOWN)
        {
            if (!isAssignCompatible(controlType, node->toExpr->dtype))
            {
                assignIncompatibleError(controlType, node->toExpr->dtype, node->varName);
            }
        }
    }

    if (node->body) visitStatement(node->body.get());

    node->dtype = DataType::VOID;
}

void SemanticAnalyzer::visitRepeat(RepeatNode *node)
{
    for (auto &stmt : node->statements)
    {
        if (stmt) visitStatement(stmt.get());
    }

    if (node->condition)
    {
        visitStatement(node->condition.get());
        if (node->condition->dtype != DataType::BOOLEAN && node->condition->dtype != DataType::UNKNOWN)
        {
            nonBooleanConditionError("repeat", node->condition->dtype);
        }
    }

    node->dtype = DataType::VOID;
}

void SemanticAnalyzer::visitCase(CaseNode *node)
{
    if (node->selector)
    {
        visitStatement(node->selector.get());
        if (node->selector->dtype != DataType::UNKNOWN && !isSimpleType(node->selector->dtype) && node->selector->dtype != DataType::BOOLEAN)
        {
            if (node->selector->dtype == DataType::REAL || node->selector->dtype == DataType::STRING || 
                node->selector->dtype == DataType::ARRAY || node->selector->dtype == DataType::RECORD)
            {
                semanticError("Case selector must be of ordinal type.");
            }
        }
    }

    for (auto &branch : node->branches)
    {
        if (branch.second) visitStatement(branch.second.get());
    }

    node->dtype = DataType::VOID;
}

void SemanticAnalyzer::visitProcCall(ProcCallNode *node)
{
    int idx = st.lookup(node->procName);
    if (idx == -1)
    {
        undeclaredError(node->procName);
        node->dtype = DataType::UNKNOWN;
        return;
    }

    AllowedObj obj = st.tab[idx].obj;
    if (obj != AllowedObj::PROCEDURE && obj != AllowedObj::FUNCTION)
    {
        wrongObjectKindError(node->procName, AllowedObj::PROCEDURE, obj);
        node->dtype = DataType::UNKNOWN;
        return;
    }

    for (auto &arg : node->args)
    {
        if (arg) visitStatement(arg.get());
    }

    if (idx == PredefinedIdx::PROC_WRITELN || idx == PredefinedIdx::PROC_READLN)
    {
        node->dtype = DataType::VOID;
        return;
    }

    int ref = st.tab[idx].ref;
    int paramCount = 0;
    int p = st.btab[ref].lpar;
    
    std::vector<int> params;
    while (p > 0)
    {
        params.push_back(p);
        p = st.tab[p].link;
    }
    
    std::reverse(params.begin(), params.end());
    paramCount = params.size();

    if ((int)node->args.size() != paramCount)
    {
        wrongArgCountError(node->procName, paramCount, node->args.size());
    }
    else
    {
        for (size_t i = 0; i < node->args.size(); i++)
        {
            if (node->args[i] && node->args[i]->dtype != DataType::UNKNOWN)
            {
                DataType expectedType = st.tab[params[i]].type;
                if (!isAssignCompatible(expectedType, node->args[i]->dtype))
                {
                    assignIncompatibleError(expectedType, node->args[i]->dtype);
                }
            }
        }
    }

    if (st.tab[idx].obj == AllowedObj::FUNCTION)
        node->dtype = st.tab[idx].type;
    else
        node->dtype = DataType::VOID;
}

void SemanticAnalyzer::visitArrayAccess(ArrayAccessNode *node) {
    if (node->array) visitStatement(node->array.get());
    
    for (auto &idx : node->indices)
        if (idx) visitStatement(idx.get());
    
    // Ambil tipe elemen dari atab
    if (node->array && node->array->dtype == DataType::ARRAY) {
        VarNode *varNode = dynamic_cast<VarNode *>(node->array.get());
        if (varNode) {
            int tabIdx = varNode->tabIndex;
            if (tabIdx >= 0) {
                int ref = st.tab[tabIdx].ref;
                if (ref >= 0 && ref < (int)st.atab.size())
                    node->dtype = st.atab[ref].etyp;
            }
        }
    } else {
        node->dtype = DataType::UNKNOWN;
    }
}

void SemanticAnalyzer::visitUnaryOp(UnaryOpNode *node)
{
    if (node->operand) visitStatement(node->operand.get());
    
    if (node->op == "notsy")
    {
        // not harus boolean
        if (node->operand && node->operand->dtype != DataType::BOOLEAN && 
            node->operand->dtype != DataType::UNKNOWN)
        {
            semanticError("Operand of 'not' must be Boolean");
        }
        node->dtype = DataType::BOOLEAN;
    }
    else
    {
        // unary + atau - harus integer atau real
        node->dtype = node->operand ? node->operand->dtype : DataType::UNKNOWN;
    }
}