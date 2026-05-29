// src/ICG.h
#pragma once
#include <vector>
#include <string>
#include <ostream>
#include "ASTNode.h"
#include "symbolTable.hpp"
#include <map>
#include <unordered_map>

using namespace std;

enum class OpCode
{
    LIT,
    LOD,
    STO,
    CAL,
    INT_OP,
    JMP,
    JPC,
    OPR,
    RET,
    LODA,
    STOA
};

enum class OprCode
{
    NEG = 1,
    ADD = 2,
    SUB = 3,
    MUL = 4,
    DIV = 5,
    MOD = 6,
    EQL = 7,
    NEQ = 8,
    LSS = 9,
    GEQ = 10,
    GTR = 11,
    LEQ = 12,
    WRT = 13,
    WRTLN = 14,
    PUSHBP = 15,
    RED = 16
};

struct Instruction
{
    int lineNo;
    OpCode opcode;
    int level;
    int operand;
};

class ICG
{
public:
    explicit ICG(SymbolTable &symTable);
    vector<Instruction> generate(ASTNodePtr root);
    std::unordered_map<int, std::string> stringTable;
    std::unordered_map<int, double> realPool;


    void printInstructions(ostream &out) const;
    void writeToFile(const string &filename) const;
    static string opcodeToString(OpCode op);

private:
    SymbolTable &st;
    vector<Instruction> instructions;
    int currentLine = 0;
    int currentLevel = 1; 
    map<string, int> funcStartLine;
    int nextStringIdx = -100000;
    int nextRealIdx = -1;
    void genString(StringNode *node);
    
    int getFuncStartLine(const string&name) const; // anggota 2
    void genProgram(ProgramNode *node);
    void genINT(ProgramNode *node); // instruksi INT
    void genStatement(ASTNode *node);
    void genAssign(AssignNode *node); // LIT / LOD / STO
    void genExpression(ASTNode *node);
    void genBinOp(BinOpNode *node);     // OPR aritmatika
    void genUnaryOp(UnaryOpNode *node); // OPR NEG
    void genVar(VarNode *node);         // LOD
    void genNumber(NumberNode *node);   // LIT
    void genChar(CharNode *node);       // LIT

    void genIf(IfNode *node);             // anggota 2
    void genWhile(WhileNode *node);       // anggota 2
    void genFor(ForNode *node);           // anggota 2
    void genProcCall(ProcCallNode *node); // anggota 2
    void genRepeat(RepeatNode *node);   
    void genCase(CaseNode *node);       
    void genProcDecl(ProcDeclNode *node); // anggota 3
    void genFuncDecl(FuncDeclNode *node); // anggota 3
    void genArrayAddress(ArrayAccessNode *an);
    void genFieldAccessAddress(FieldAccessNode *fn);
    void genRecordAddress(ASTNode *node);

    void emit(OpCode op, int level, int operand);
    int countVarDecl(ProgramNode *node);
    void genNestedSubprograms(const vector<ASTNodePtr> &declaration);
    int findReturnVarAddr(FuncDeclNode *node);
};