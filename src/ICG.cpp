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

    for (auto &decl : node->declarations)
    {
        if (auto *pd = dynamic_cast<ProcDeclNode *>(decl.get()))
            genProcDecl(pd);
        else if (auto *fd = dynamic_cast<FuncDeclNode *>(decl.get()))
            genFuncDecl(fd);
    }

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
    else if (auto *an = dynamic_cast<ArrayAccessNode *>(target))
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
    if (op == "+")
        emit(OpCode::OPR, 0, (int)OprCode::ADD);
    else if (op == "-")
        emit(OpCode::OPR, 0, (int)OprCode::SUB);
    else if (op == "*")
        emit(OpCode::OPR, 0, (int)OprCode::MUL);
    else if (op == "div")
        emit(OpCode::OPR, 0, (int)OprCode::DIV);
    else if (op == "/")
        emit(OpCode::OPR, 0, (int)OprCode::DIV);
    else if (op == "mod")
        emit(OpCode::OPR, 0, (int)OprCode::MOD);

    else if (op == "=")
        emit(OpCode::OPR, 0, (int)OprCode::EQL);
    else if (op == "<>")
        emit(OpCode::OPR, 0, (int)OprCode::NEQ);
    else if (op == "<")
        emit(OpCode::OPR, 0, (int)OprCode::LSS);
    else if (op == ">=")
        emit(OpCode::OPR, 0, (int)OprCode::GEQ);
    else if (op == ">")
        emit(OpCode::OPR, 0, (int)OprCode::GTR);
    else if (op == "<=")
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
void ICG::genProcDecl(ProcDeclNode *) { /* TODO: anggota 3 */ }
void ICG::genFuncDecl(FuncDeclNode *) { /* TODO: anggota 3 */ }

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