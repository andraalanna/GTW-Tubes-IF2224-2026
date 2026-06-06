#include "ICG.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

using namespace std;

ICG::ICG(SymbolTable &symTable)
    : st(symTable), currentLine(0), nextRealIdx(-1) {}

vector<Instruction> ICG::generate(ASTNodePtr root)
{
    instructions.clear();
    currentLine = 0;
    currentLevel = 1; 

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
    // Gunakan vsze dari btab yang sudah dihitung Semantic Analyzer.
    // vsze = total ukuran semua variabel (benar untuk array, record, dll.)
    int ref  = st.tab[node->tabIndex].ref;
    int vsze = (ref >= 0 && ref < (int)st.btab.size())
               ? st.btab[ref].vsze
               : 0;
    int memSize = 3 + vsze;
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
    else if (auto *n = dynamic_cast<RepeatNode *>(node))
        genRepeat(n);
    else if (auto *n = dynamic_cast<CaseNode *>(node))
        genCase(n);
    else if (auto *n = dynamic_cast<ProcCallNode *>(node))
        genProcCall(n);
    else if (auto *n = dynamic_cast<ProcDeclNode *>(node))
    {
        int jmpPos = currentLine;
        emit(OpCode::JMP, 0, 0); // Skip over procedure body
        genProcDecl(n);
        instructions[jmpPos].operand = currentLine;
    }
    else if (auto *n = dynamic_cast<FuncDeclNode *>(node))
    {
        int jmpPos = currentLine;
        emit(OpCode::JMP, 0, 0); // Skip over function body
        genFuncDecl(n);
        instructions[jmpPos].operand = currentLine;
    }
    else if (auto *n = dynamic_cast<VarDeclNode *>(node))
    {
        (void)n;
    }
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
        int addr     = st.tab[vn->tabIndex].adr;
        int varLevel = st.tab[vn->tabIndex].lev;
        int relLevel = currentLevel - varLevel;

        if (st.tab[vn->tabIndex].obj == AllowedObj::FUNCTION)
        {
            int ref  = st.tab[vn->tabIndex].ref;
            int psze = (ref > 0 && ref < (int)st.btab.size())
                       ? st.btab[ref].psze
                       : (int)0;
            int vsze = (ref > 0 && ref < (int)st.btab.size())  // ← TAMBAH
                       ? st.btab[ref].vsze
                       : 0;
            emit(OpCode::STO, 0, 3 + psze + vsze);
            return;
        }

        addr = resolveAddr(vn->tabIndex);

        emit(OpCode::STO, relLevel, addr);
    }
    else if (auto *an = dynamic_cast<ArrayAccessNode *>(target))
    {
        genArrayAddress(an);
        emit(OpCode::STOA, 0, 0);
    }
    else if (auto *fn = dynamic_cast<FieldAccessNode *>(target))
    {
        genFieldAccessAddress(fn);
        emit(OpCode::STOA, 0, 0);
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
    else if (auto *n = dynamic_cast<StringNode *>(node))
        genString(n);
    else if (auto *n = dynamic_cast<VarNode *>(node))
        genVar(n);
    else if (auto *n = dynamic_cast<BinOpNode *>(node))
        genBinOp(n);
    else if (auto *n = dynamic_cast<UnaryOpNode *>(node))
        genUnaryOp(n);
    else if (auto *n = dynamic_cast<ProcCallNode *>(node))  // ← TAMBAH
        genProcCall(n);        
    else if (auto *an = dynamic_cast<ArrayAccessNode *>(node))
    {
        genArrayAddress(an);
        emit(OpCode::LODA, 0, 0); 
    }
    else if (auto *fn = dynamic_cast<FieldAccessNode *>(node))
    {
        genFieldAccessAddress(fn);
        emit(OpCode::LODA, 0, 0);
    }
}

void ICG::genNumber(NumberNode *node)
{
    if (node->dtype == DataType::REAL)
    {
        double f = stod(node->rawValue);
        int idx = -1;
        for (auto &[k, v] : realPool)
        {
            if (v == f)
            {
                idx = k;
                break;
            }
        }
        if (idx == -1)
        {
            idx = nextRealIdx--;
            realPool[idx] = f;
        }
        emit(OpCode::LIT, static_cast<int>(DataType::REAL), idx);
    }
    else
    {
        int value = stoi(node->rawValue);
        emit(OpCode::LIT, static_cast<int>(DataType::INTEGER), value);
    }
}

void ICG::genChar(CharNode *node)
{
    char c = 0;
    if (!node->rawValue.empty())
    {
        if (node->rawValue.size() >= 3 && node->rawValue[0] == '\'')
            c = node->rawValue[1];
        else
            c = node->rawValue[0];
    }
    emit(OpCode::LIT, static_cast<int>(DataType::CHAR), (int)c);
}

void ICG::genVar(VarNode *node)
{
    int idx = node->tabIndex;
    if (st.tab[idx].obj == AllowedObj::CONSTANT)
    {
        emit(OpCode::LIT, 0, st.tab[idx].adr);
        return;
    }

    int addr     = resolveAddr(idx);           // ← ganti
    int varLevel = st.tab[idx].lev;
    int relLevel = currentLevel - varLevel;

    emit(OpCode::LOD, relLevel, addr);
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
    else if (op == "slash" || op == "rdiv")
        emit(OpCode::OPR, 0, (int)OprCode::DIV);
    else if (op == "div" || op == "idiv")
        emit(OpCode::OPR, 0, (int)OprCode::DIV);
    else if (op == "mod" || op == "imod")
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
    if (node->op == "-" || node->op == "minus")
        emit(OpCode::OPR, 0, (int)OprCode::NEG);
    else if (node->op == "not" || node->op == "notsy")
    {
        emit(OpCode::LIT, 0, 0);
        emit(OpCode::OPR, 0, (int)OprCode::EQL);
    }
}

void ICG::genIf(IfNode *node)
{
    if (!node)
        return;
    genExpression(node->condition.get());
 
    int jpcPos = currentLine;
    emit(OpCode::JPC, 0, 0); // placeholder
 
    genStatement(node->thenBranch.get());
 
    if (node->elseBranch)
    {
        int jmpPos = currentLine;
        emit(OpCode::JMP, 0, 0); // placeholder
        instructions[jpcPos].operand = currentLine;
        genStatement(node->elseBranch.get());
        instructions[jmpPos].operand = currentLine;
    }
    else
    {
        instructions[jpcPos].operand = currentLine;
    }
}

void ICG::genWhile(WhileNode *node)
{
    if (!node)
        return;
 
    int loopStart = currentLine;
 
    genExpression(node->condition.get());
    int jpcPos = currentLine;
    emit(OpCode::JPC, 0, 0); // placeholder
 
    genStatement(node->body.get());
    emit(OpCode::JMP, 0, loopStart);
 
    instructions[jpcPos].operand = currentLine;
}

void ICG::genFor(ForNode *node)
{
    if (!node)
        return;
 
    int varAddr  = -1;
    int varLevel = -1;
    int varIdx   = -1; 
 
    for (int i = 0; i < (int)st.tab.size(); i++)
    {
        if (st.tab[i].name == node->varName &&
            st.tab[i].obj  == AllowedObj::VARIABLE)
        {
            varAddr  = st.tab[i].adr;
            varLevel = st.tab[i].lev;
            varIdx   = i;
        }
    }
 
    if (varAddr < 0)
        throw runtime_error("ICG genFor: variabel loop '" + node->varName + "' tidak ditemukan di symbol table");

    int relLevel = currentLevel - varLevel;

    varAddr = resolveAddr(varIdx);   

    genExpression(node->fromExpr.get());
    emit(OpCode::STO, relLevel, varAddr);
 
    int loopStart = currentLine;
    emit(OpCode::LOD, relLevel, varAddr);   
    genExpression(node->toExpr.get());
 
    if (node->isDownto)
        emit(OpCode::OPR, 0, (int)OprCode::GEQ);
    else
        emit(OpCode::OPR, 0, (int)OprCode::LEQ);
 
    int jpcPos = currentLine;
    emit(OpCode::JPC, 0, 0);
 
    genStatement(node->body.get());
 
    emit(OpCode::LOD, relLevel, varAddr);    
    emit(OpCode::LIT, 0, 1);
    if (node->isDownto)
        emit(OpCode::OPR, 0, (int)OprCode::SUB);
    else
        emit(OpCode::OPR, 0, (int)OprCode::ADD);
    emit(OpCode::STO, relLevel, varAddr);   
 
    emit(OpCode::JMP, 0, loopStart);
    instructions[jpcPos].operand = currentLine;
}

void ICG::genRepeat(RepeatNode *node)
{
    if (!node)
        return;
 
    int loopStart = currentLine;
 
    for (auto &stmt : node->statements)
        genStatement(stmt.get());
 
    genExpression(node->condition.get());
 
    emit(OpCode::JPC, 0, loopStart);
}

void ICG::genCase(CaseNode *node)
{
    if (!node)
        return;
 
    vector<int> jmpAfterPositions;
 
    for (auto &branch : node->branches)
    {
 
        vector<int> jmpToBodyPositions;
 
        for (const string &val : branch.first)
        {

            genExpression(node->selector.get());
 
            try {
                int v = stoi(val);
                emit(OpCode::LIT, 0, v);
            } catch (...) {
                char c = val.empty() ? 0 : val[0];
                emit(OpCode::LIT, 0, (int)c);
            }
 
            emit(OpCode::OPR, 0, (int)OprCode::EQL);
 
            int jpcToNextPos = currentLine;
            emit(OpCode::JPC, 0, 0); 
 
            int jmpToBodyPos = currentLine;
            emit(OpCode::JMP, 0, 0);
            jmpToBodyPositions.push_back(jmpToBodyPos);
            instructions[jpcToNextPos].operand = currentLine;
        }
 
        int jmpToNextBranch = currentLine;
        emit(OpCode::JMP, 0, 0); 
 
        int bodyStart = currentLine;
        for (int pos : jmpToBodyPositions)
            instructions[pos].operand = bodyStart;
 
        genStatement(branch.second.get());
 
        int jmpAfterPos = currentLine;
        emit(OpCode::JMP, 0, 0); // placeholder
        jmpAfterPositions.push_back(jmpAfterPos);

        instructions[jmpToNextBranch].operand = currentLine;
    }

    int afterPos = currentLine;
    for (int pos : jmpAfterPositions)
        instructions[pos].operand = afterPos;
}

void ICG::genProcCall(ProcCallNode *node)
{
    if (!node)
        return;
 
    const string &name = node->procName;
 
    if (name == "write")
    {
        for (auto &arg : node->args)
        {
            genExpression(arg.get());
            emit(OpCode::OPR, static_cast<int>(arg->dtype), (int)OprCode::WRT);
        }
        return;
    }

    if (name == "writeln")
    {
        if (node->args.empty())
        {

            emit(OpCode::LIT, 0, 0);
            emit(OpCode::OPR, static_cast<int>(DataType::VOID), (int)OprCode::WRTLN);
        }
        else
        {

            for (int i = 0; i < (int)node->args.size(); i++)
            {
                genExpression(node->args[i].get());
                if (i == (int)node->args.size() - 1)
                    emit(OpCode::OPR, static_cast<int>(node->args[i]->dtype), (int)OprCode::WRTLN); 
                else
                    emit(OpCode::OPR, static_cast<int>(node->args[i]->dtype), (int)OprCode::WRT);
            }
        }
        return;
    }

    if (name == "read" || name == "readln")
    {
        for (auto &arg : node->args)
        {
            genRecordAddress(arg.get());
            emit(OpCode::OPR, static_cast<int>(arg->dtype), (int)OprCode::RED);
        }
        return;
    }
 
    int procTabIdx = node->tabIndex;
    if (procTabIdx < 0)
    {
        for (int i = 0; i < (int)st.tab.size(); i++)
        {
            if (st.tab[i].name == name &&
                (st.tab[i].obj == AllowedObj::PROCEDURE ||
                 st.tab[i].obj == AllowedObj::FUNCTION))
            {
                procTabIdx = i;
                break;
            }
        }
    }
 
    if (procTabIdx < 0)
        throw runtime_error("ICG genProcCall: prosedur '" + name + "' tidak ditemukan di symbol table");

    for (auto &arg : node->args)
        genExpression(arg.get());
 
    int procDefLevel = st.tab[procTabIdx].lev;
    int levelDiff = currentLevel - procDefLevel;
 
    
    int targetLine = st.tab[procTabIdx].adr;
    if (targetLine <= 0)
    {
        targetLine = getFuncStartLine(name);
    }
 
    emit(OpCode::CAL, levelDiff, targetLine);
}
 

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

    for (int p = 0; p < psze; p++)
    {
        emit(OpCode::LOD, 0, -(psze - p));  // load arg dari bp-(psze-p)
        emit(OpCode::STO, 0, 3 + p);        // simpan ke bp+3+p
    }

    currentLevel++; 

    if (node->body)
        genStatement(node->body.get());
    currentLevel--;

    emit(OpCode::RET, 0, psze);
}

void ICG::genFuncDecl(FuncDeclNode *node)
{
    int startLine = currentLine;
    st.tab[node->tabIndex].adr = startLine;

    funcStartLine[node->funcName] = startLine;
    int ref       = st.tab[node->tabIndex].ref;
    int psze      = (ref > 0 && ref < (int)st.btab.size())
                    ? st.btab[ref].psze : (int)node->params.size();
    int vsze      = (ref > 0 && ref < (int)st.btab.size())
                    ? st.btab[ref].vsze : 0;   

    int frameSize = 3 + psze + vsze + 1;      

    emit(OpCode::INT_OP, 0, frameSize);

    for (int p = 0; p < psze; p++)
    {
        emit(OpCode::LOD, 0, -(psze - p));
        emit(OpCode::STO, 0, 3 + p);
    }

    currentLevel++;
    if (node->body)
        genStatement(node->body.get());
    currentLevel--;

    int resultAdr = findReturnVarAddr(node);
    emit(OpCode::LOD, 0, resultAdr);
    emit(OpCode::RET, 1, psze);
}

void ICG::genArrayAddress(ArrayAccessNode *an)
{
    genRecordAddress(an->array.get());

    int ref = 0;
    if (auto *vn = dynamic_cast<VarNode *>(an->array.get()))
    {
        ref = st.tab[vn->tabIndex].ref;
    }
    else
    {
        ref = an->array->typeRef;
    }

    for (size_t k = 0; k < an->indices.size(); k++)
    {
        int low = 0;
        int high = 0;
        int elsz = 1;
        if (ref >= 0 && ref < (int)st.atab.size())
        {
            low = st.atab[ref].low;
            high = st.atab[ref].high;
            elsz = st.atab[ref].elsz;
            ref = st.atab[ref].eref;
        }

        genExpression(an->indices[k].get());

        emit(OpCode::CKB, low, high);

        emit(OpCode::LIT, 0, low);
        emit(OpCode::OPR, 0, (int)OprCode::SUB);

        if (elsz > 1)
        {
            emit(OpCode::LIT, 0, elsz);
            emit(OpCode::OPR, 0, (int)OprCode::MUL);
        }

        emit(OpCode::OPR, 0, (int)OprCode::ADD);
    }
}


void ICG::emit(OpCode op, int level, int operand)
{
    instructions.push_back({currentLine, op, level, operand});
    currentLine++;
}

string ICG::opcodeToString(OpCode op) 
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
    case OpCode::LODA:
        return "LODA";
    case OpCode::STOA:
        return "STOA";
    case OpCode::CKB:
        return "CKB";
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
            return st.tab[i].adr + 3; 
        }
    }

    // Fallback 1: cari variabel bernama "result"
    for (int i = PredefinedIdx::USER_START; i < (int)st.tab.size(); i++)
    {
        if (st.tab[i].obj  == AllowedObj::VARIABLE &&
            st.tab[i].lev  == bodyLev &&
            st.tab[i].name == "result")
        {
            return st.tab[i].adr + 3; // ← TAMBAH +3
        }
    }

    // Fallback 2: hitung manual dari btab
    int ref  = st.tab[node->tabIndex].ref;
    int psze = (ref > 0 && ref < (int)st.btab.size())
               ? st.btab[ref].psze
               : (int)node->params.size();
    int vsze = (ref > 0 && ref < (int)st.btab.size())  // ← TAMBAH
               ? st.btab[ref].vsze
               : 0;
    return 3 + psze + vsze; // sudah benar, tidak perlu diubah
}

int ICG::getFuncStartLine(const string& name) const{
    auto it = funcStartLine.find(name);
    if (it == funcStartLine.end()) throw runtime_error("ICG: subprogram '" + name + "' belum di-generate. " "Pastikan genProcDecl/genFuncDecl dipanggil sebelum genProcCall.");
    return it->second;
}

void ICG::genString(StringNode *node)
{
    const std::string &s = node->rawValue;

    int idx = -1;
    for (auto &[k, v] : stringTable)
        if (v == s)
        {
            idx = k;
            break;
        }

    if (idx == -1)
    {
        idx = nextStringIdx--;
        stringTable[idx] = s;
    }

    emit(OpCode::LIT, static_cast<int>(DataType::STRING), idx);
}

void ICG::genFieldAccessAddress(FieldAccessNode *fn)
{
    genRecordAddress(fn->record.get());
    int offset = st.tab[fn->tabIndex].adr;
    if (offset != 0)
    {
        emit(OpCode::LIT, 0, offset);
        emit(OpCode::OPR, 0, (int)OprCode::ADD);
    }
}

void ICG::genRecordAddress(ASTNode *node)
{
    if (auto *vn = dynamic_cast<VarNode *>(node))
    {
        int baseAddr = st.tab[vn->tabIndex].adr;
        int varLevel = st.tab[vn->tabIndex].lev;
        if (varLevel >= 2)
            baseAddr += 3;

        emit(OpCode::OPR, 0, (int)OprCode::PUSHBP);
        int relLevel = currentLevel - varLevel;
        for (int i = 0; i < relLevel; i++)
        {
            emit(OpCode::LODA, 0, 0);
        }
        emit(OpCode::LIT, 0, baseAddr);
        emit(OpCode::OPR, 0, (int)OprCode::ADD);
    }
    else if (auto *an = dynamic_cast<ArrayAccessNode *>(node))
    {
        genArrayAddress(an);
    }
    else if (auto *fn = dynamic_cast<FieldAccessNode *>(node))
    {
        genFieldAccessAddress(fn);
    }
    else
    {
        throw std::runtime_error("ICG: Unsupported base for address generation");
    }
}

int ICG::resolveAddr(int tabIdx)
{
    int varLevel = st.tab[tabIdx].lev;
    int addr     = st.tab[tabIdx].adr;

    if (varLevel < 2)
        return addr; // program utama: tidak ada offset

    // Cari btab scope variabel ini (level = varLevel)
    for (int b = 0; b < (int)st.btab.size(); b++)
    {
        int lpar = st.btab[b].lpar;
        int psze = st.btab[b].psze;

        // Cek apakah tabIdx ada di linked list parameter blok ini
        bool isParam = false;
        int cur = lpar;
        while (cur > 0)
        {
            if (cur == tabIdx) { isParam = true; break; }
            cur = st.tab[cur].link;
        }

        // Cek apakah tabIdx ada di blok ini sama sekali
        bool inThisBlock = false;
        cur = st.btab[b].last;
        while (cur > 0)
        {
            if (cur == tabIdx) { inThisBlock = true; break; }
            cur = st.tab[cur].link;
        }

        if (!inThisBlock) continue;

        if (isParam)
            return 3 + addr;         // parameter: bp+3, bp+4, ...
        else
            return 3 + psze + addr;  // variabel lokal: bp+3+psze, bp+4+psze, ...
    }

    return 3 + addr; // fallback
}