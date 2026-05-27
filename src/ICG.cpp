#include "ICG.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

using namespace std;

ICG::ICG(SymbolTable &symTable)
    : st(symTable), currentLine(0) {}

vector<Instruction> ICG::generate(ASTNodePtr root)
{
    instructions.clear();
    currentLine = 0;

    auto *prog = dynamic_cast<ProgramNode *>(root.get());
    if (!prog)
        throw runtime_error("ICG: root bukan ProgramNode");

    genProgram(prog);
    return instructions;
}

void ICG::genProgram(ProgramNode *node)
{
    genINT(node);
    int jmpPos = currentLine;
    emit(OpCode::JMP, 0, 0);

    for (auto &decl : node->declarations)
    {
        if (auto *pd = dynamic_cast<ProcDeclNode *>(decl.get()))
            genProcDecl(pd);
        else if (auto *fd = dynamic_cast<FuncDeclNode *>(decl.get()))
            genFuncDecl(fd);
    }

    instructions[jmpPos].operand = currentLine;

    if (node->body)
    {
        auto *compound = dynamic_cast<CompoundStmtNode *>(node->body.get());
        if (compound)
        {
            for (auto &stmt : compound->statements)
                genStatement(stmt.get());
        }
    }

    emit(OpCode::RET, 0, 0);
}

int ICG::countVarDecl(ProgramNode *node)
{
    int count = 0;
    for (auto &decl : node->declarations)
    {
        if (dynamic_cast<VarDeclNode *>(decl.get()))
            count++;
    }
    return count;
}

void ICG::genINT(ProgramNode *node)
{
    int memSize = 3 + countVarDecl(node);
    emit(OpCode::INT_OP, 0, memSize);
}

void ICG::genStatement(ASTNode *node)
{
    if (!node)
        return;

    if (auto *n = dynamic_cast<AssignNode *>(node))
        genAssign(n);
    else if (auto *n = dynamic_cast<IfNode *>(node))
        genIf(n);
    else if (auto *n = dynamic_cast<WhileNode *>(node))
        genWhile(n);
    else if (auto *n = dynamic_cast<ForNode *>(node))
        genFor(n);
    else if (auto *n = dynamic_cast<ProcCallNode *>(node))
        genProcCall(n);
    else if (auto *n = dynamic_cast<CompoundStmtNode *>(node))
    {
        for (auto &s : n->statements)
            genStatement(s.get());
    }
}

void ICG::genAssign(AssignNode *node)
{
    genExpression(node->value.get());
    auto *target = node->target.get();

    if (auto *vn = dynamic_cast<VarNode *>(target))
    {
        int addr = st.tab[vn->tabIndex].adr;
        int level = st.tab[vn->tabIndex].lev;
        emit(OpCode::STO, level, addr);
    }
    else if (dynamic_cast<ArrayAccessNode *>(target))
    {
        // TODO: koordinasi sama anggota 3/4
    }
}

void ICG::genExpression(ASTNode *node)
{
    if (!node)
        return;

    if (auto *n = dynamic_cast<NumberNode *>(node))
        genNumber(n);
    else if (auto *n = dynamic_cast<CharNode *>(node))
        genChar(n);
    else if (auto *n = dynamic_cast<VarNode *>(node))
        genVar(n);
    else if (auto *n = dynamic_cast<BinOpNode *>(node))
        genBinOp(n);
    else if (auto *n = dynamic_cast<UnaryOpNode *>(node))
        genUnaryOp(n);
}

void ICG::genNumber(NumberNode *node)
{
    int value;
    if (node->dtype == DataType::REAL)
        value = (int)stof(node->rawValue);
    else
        value = stoi(node->rawValue);
    emit(OpCode::LIT, 0, value);
}

void ICG::genChar(CharNode *node)
{
    char c = node->rawValue.empty() ? 0 : node->rawValue[0];
    emit(OpCode::LIT, 0, (int)c);
}

void ICG::genVar(VarNode *node)
{
    int addr = st.tab[node->tabIndex].adr;
    int level = st.tab[node->tabIndex].lev;
    emit(OpCode::LOD, level, addr);
}

void ICG::genBinOp(BinOpNode *node)
{
    genExpression(node->left.get());
    genExpression(node->right.get());
    string op = node->op;

    // Aritmatika — token names dari Parser/ASTBuilder
    if (op == "plus")
        emit(OpCode::OPR, 0, (int)OprCode::ADD);
    else if (op == "minus")
        emit(OpCode::OPR, 0, (int)OprCode::SUB);
    else if (op == "times")
        emit(OpCode::OPR, 0, (int)OprCode::MUL);
    else if (op == "slash")
        emit(OpCode::OPR, 0, (int)OprCode::DIV);
    else if (op == "div")
        emit(OpCode::OPR, 0, (int)OprCode::DIV);
    else if (op == "mod")
        emit(OpCode::OPR, 0, (int)OprCode::MOD);
    // Perbandingan
    else if (op == "eql")
        emit(OpCode::OPR, 0, (int)OprCode::EQL);
    else if (op == "neq")
        emit(OpCode::OPR, 0, (int)OprCode::NEQ);
    else if (op == "lss")
        emit(OpCode::OPR, 0, (int)OprCode::LSS);
    else if (op == "geq")
        emit(OpCode::OPR, 0, (int)OprCode::GEQ);
    else if (op == "gtr")
        emit(OpCode::OPR, 0, (int)OprCode::GTR);
    else if (op == "leq")
        emit(OpCode::OPR, 0, (int)OprCode::LEQ);
}

void ICG::genUnaryOp(UnaryOpNode *node)
{
    genExpression(node->operand.get());
    if (node->op == "-")
        emit(OpCode::OPR, 0, (int)OprCode::NEG);
    else if (node->op == "not")
    {
        emit(OpCode::LIT, 0, 0);
        emit(OpCode::OPR, 0, (int)OprCode::EQL);
    }
}

void ICG::genIf(IfNode *) { /* TODO: anggota 2 */ }
void ICG::genWhile(WhileNode *) { /* TODO: anggota 2 */ }
void ICG::genFor(ForNode *) { /* TODO: anggota 2 */ }
void ICG::genProcCall(ProcCallNode *) { /* TODO: anggota 2 (writeln) */ }
void ICG::genProcDecl(ProcDeclNode *node)
{

    int startLine = currentLine;
    st.tab[node->tabIndex].adr = startLine;

    funcStartLine[node->procName] = startLine;
    int ref       = st.tab[node->tabIndex].ref;
    int psze      = (ref > 0 && ref < (int)st.btab.size()) ? st.btab[ref].psze : (int)node->params.size();
    int vsze      = (ref > 0 && ref < (int)st.btab.size()) ? st.btab[ref].vsze : 0;
    int frameSize = 3 + psze + vsze;

    emit(OpCode::INT_OP, 0, frameSize);

    if (node->body)
        genStatement(node->body.get());


    emit(OpCode::RET, 0, 0);
}

void ICG::genFuncDecl(FuncDeclNode * node) 
{
    int startLine = currentLine;
    st.tab[node->tabIndex].adr = startLine;

    funcStartLine[node->funcName] = startLine;
    int ref = st.tab[node->tabIndex].ref;
    int psze      = (ref > 0 && ref < (int)st.btab.size()) ? st.btab[ref].psze : (int)node->params.size();
    int vsze      = (ref > 0 && ref < (int)st.btab.size()) ? st.btab[ref].vsze : 1;
    int frameSize = 3 + psze + vsze;

    emit(OpCode::INT_OP, 0, frameSize);

    if (node->body) genStatement(node->body.get());

    int resultAdr = findReturnVarAddr(node);
    emit(OpCode::LOD, 0, resultAdr);
    emit(OpCode::RET, 0 , 0);
}

void ICG::emit(OpCode op, int level, int operand)
{
    instructions.push_back({currentLine, op, level, operand});
    currentLine++;
}

string ICG::opcodeToString(OpCode op) const
{
    switch (op)
    {
    case OpCode::LIT:
        return "LIT";
    case OpCode::LOD:
        return "LOD";
    case OpCode::STO:
        return "STO";
    case OpCode::CAL:
        return "CAL";
    case OpCode::INT_OP:
        return "INT";
    case OpCode::JMP:
        return "JMP";
    case OpCode::JPC:
        return "JPC";
    case OpCode::OPR:
        return "OPR";
    case OpCode::RET:
        return "RET";
    default:
        return "???";
    }
}

void ICG::printInstructions(ostream &out) const
{
    for (const auto &instr : instructions)
    {
        out << instr.lineNo << " "
            << opcodeToString(instr.opcode) << " "
            << instr.level << " "
            << instr.operand << "\n";
    }
}

void ICG::writeToFile(const string &filename) const
{
    ofstream f(filename);
    if (!f)
        throw runtime_error("ICG: tidak bisa buat file " + filename);
    printInstructions(f);
}

/**
 * HELPER
 */

// Tugas: hitung ukuran frame untuk satu blok subprogram
// Frame: 3 slot wajib (static + dynamic link, return adr), jml parimeter, jml var lokal

static int countLocalVars(const vector<ASTNodePtr> &declarations){
    int count = 0;
    for (auto &decl : declarations){
         if(dynamic_cast<VarDeclNode *>(decl.get()))
            count++;
    }
    return count;
}

void ICG::genNestedSubprograms(const vector<ASTNodePtr> &declarations){
    for (auto &decl : declarations){
        if (auto *pd = dynamic_cast<ProcDeclNode *>(decl.get())) 
            genProcDecl(pd);
        else if (auto *fd = dynamic_cast<FuncDeclNode*>(decl.get()))
            genFuncDecl(fd);
    }
}

int ICG::findReturnVarAddr(FuncDeclNode *node)
{
    int funcLev = st.tab[node->tabIndex].lev;
    int bodyLev = funcLev + 1;

    // Cari variabel dengan nama = nama fungsi 
    for (int i = PredefinedIdx::USER_START; i < (int)st.tab.size(); i++)
    {
        if (st.tab[i].obj  == AllowedObj::VARIABLE &&
            st.tab[i].lev  == bodyLev &&
            st.tab[i].name == node->funcName)
        {
            return st.tab[i].adr;
        }
    }

    // Fallback 1: cari variabel bernama "result"
    for (int i = PredefinedIdx::USER_START; i < (int)st.tab.size(); i++)
    {
        if (st.tab[i].obj  == AllowedObj::VARIABLE &&
            st.tab[i].lev  == bodyLev &&
            st.tab[i].name == "result")
        {
            return st.tab[i].adr;
        }
    }

    // Fallback 2: hitung manual dari btab
    int ref  = st.tab[node->tabIndex].ref;
    int psze = (ref > 0 && ref < (int)st.btab.size()) ? st.btab[ref].psze : (int)node->params.size();
    return 3 + psze;
}

int ICG::getFuncStartLine(const string& name) const{
    auto it = funcStartLine.find(name);
    if (it == funcStartLine.end()) throw runtime_error("ICG: subprogram '" + name + "' belum di-generate. " "Pastikan genProcDecl/genFuncDecl dipanggil sebelum genProcCall.");
    return it->second;
}