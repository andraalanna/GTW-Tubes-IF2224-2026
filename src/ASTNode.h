
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "symbolTable.hpp"

using namespace std;

struct ASTNode
{
    DataType dtype = DataType::UNKNOWN;
    int tabIndex = -1;
    int lexLevel = 0;
    int typeRef = 0;

    virtual ~ASTNode() = default;

    virtual void print(ostream &out,
                       const string &prefix = "",
                       bool isLast = true,
                       bool isRoot = true) const = 0;

    // Nama jenis node untuk print
    virtual string nodeName() const = 0;

    virtual string toSource() const { return ""; }

protected:
    void printHeader(ostream &out,
                     const string &prefix,
                     bool isLast,
                     bool isRoot,
                     const string &extra = "") const;
    void printChildren(ostream &out,
                       const string &prefix,
                       bool isLast,
                       bool isRoot,
                       const vector<shared_ptr<ASTNode>> &children) const;

    string annotation() const;
};

using ASTNodePtr = shared_ptr<ASTNode>;

struct ProgramNode : ASTNode
{
    string name;
    vector<ASTNodePtr> declarations;
    ASTNodePtr body;

    explicit ProgramNode(const string &n) : name(n) {}
    string nodeName() const override { return "ProgramNode(name: '" + name + "')"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct VarDeclNode : ASTNode
{
    string varName;
    DataType varType;
    int typeRef = 0;

    VarDeclNode(const string &n, DataType t, int ref = 0)
        : varName(n), varType(t), typeRef(ref) { dtype = t; }
    string nodeName() const override { return "VarDecl('" + varName + "')"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct ConstDeclNode : ASTNode
{
    string constName;
    DataType constType;
    string value;

    ConstDeclNode(const string &n, DataType t, const string &v)
        : constName(n), constType(t), value(v) { dtype = t; }
    string nodeName() const override { return "ConstDecl(" + constName + " = " + value + ")"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct TypeDeclNode : ASTNode
{
    string typeName;
    DataType baseType;
    int typeRef = 0;

    TypeDeclNode(const string &n, DataType t, int ref = 0)
        : typeName(n), baseType(t), typeRef(ref) { dtype = t; }
    string nodeName() const override { return "TypeDecl(" + typeName + ")"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct ProcDeclNode : ASTNode
{
    string procName;
    vector<ASTNodePtr> params;
    ASTNodePtr body;

    explicit ProcDeclNode(const string &n) : procName(n) { dtype = DataType::VOID; }
    string nodeName() const override { return "ProcDecl(" + procName + ")"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct FuncDeclNode : ASTNode
{
    string funcName;
    DataType returnType;
    vector<ASTNodePtr> params;
    ASTNodePtr body;

    FuncDeclNode(const string &n, DataType r)
        : funcName(n), returnType(r) { dtype = r; }
    string nodeName() const override { return "FuncDecl(" + funcName + ")"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct CompoundStmtNode : ASTNode
{
    vector<ASTNodePtr> statements;

    CompoundStmtNode() { dtype = DataType::VOID; }
    string nodeName() const override { return "CompoundStmt"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct AssignNode : ASTNode
{
    ASTNodePtr target;
    ASTNodePtr value;

    AssignNode(ASTNodePtr t, ASTNodePtr v)
        : target(move(t)), value(move(v)) { dtype = DataType::VOID; }
    string nodeName() const override { 
        string tStr = target ? target->toSource() : "";
        string vStr = value ? value->toSource() : "";
        if (tStr.empty() || vStr.empty()) return "Assign";
        return "Assign(" + tStr + " := " + vStr + ")"; 
    }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct BinOpNode : ASTNode
{
    string op;
    ASTNodePtr left;
    ASTNodePtr right;

    BinOpNode(const string &o, ASTNodePtr l, ASTNodePtr r)
        : op(o), left(move(l)), right(move(r)) {}
    string nodeName() const override { return "BinOp(" + op + ")"; }
    string toSource() const override;
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct UnaryOpNode : ASTNode
{
    string op;
    ASTNodePtr operand;

    UnaryOpNode(const string &o, ASTNodePtr opnd)
        : op(o), operand(move(opnd)) {}
    string nodeName() const override { return "UnaryOp(" + op + ")"; }
    string toSource() const override;
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct VarNode : ASTNode
{
    string varName;

    explicit VarNode(const string &n) : varName(n) {}
    string nodeName() const override { return "Var(" + varName + ")"; }
    string toSource() const override { return varName; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct NumberNode : ASTNode
{
    string rawValue;

    NumberNode(const string &v, DataType t) : rawValue(v) { dtype = t; }
    string nodeName() const override { return "Num(" + rawValue + ")"; }
    string toSource() const override { return rawValue; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct CharNode : ASTNode
{
    string rawValue;

    explicit CharNode(const string &v) : rawValue(v) { dtype = DataType::CHAR; }
    string nodeName() const override { return "Char(" + rawValue + ")"; }
    string toSource() const override { return "'" + rawValue + "'"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct StringNode : ASTNode
{
    string rawValue;

    explicit StringNode(const string &v) : rawValue(v) { dtype = DataType::STRING; }
    string nodeName() const override { return "Str(" + rawValue + ")"; }
    string toSource() const override { return "\"" + rawValue + "\""; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct ProcCallNode : ASTNode
{
    string procName;
    vector<ASTNodePtr> args;

    explicit ProcCallNode(const string &n) : procName(n) {}
    string nodeName() const override { return "ProcedureCall(name: '" + procName + "')"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct IfNode : ASTNode
{
    ASTNodePtr condition;
    ASTNodePtr thenBranch;
    ASTNodePtr elseBranch;

    IfNode(ASTNodePtr c, ASTNodePtr t, ASTNodePtr e = nullptr)
        : condition(move(c)), thenBranch(move(t)), elseBranch(move(e))
    {
        dtype = DataType::VOID;
    }
    string nodeName() const override { return "If"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct WhileNode : ASTNode
{
    ASTNodePtr condition;
    ASTNodePtr body;

    WhileNode(ASTNodePtr c, ASTNodePtr b)
        : condition(move(c)), body(move(b)) { dtype = DataType::VOID; }
    string nodeName() const override { return "While"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct RepeatNode : ASTNode
{
    vector<ASTNodePtr> statements;
    ASTNodePtr condition;

    RepeatNode(vector<ASTNodePtr> s, ASTNodePtr c)
        : statements(move(s)), condition(move(c)) { dtype = DataType::VOID; }
    string nodeName() const override { return "Repeat"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct ForNode : ASTNode
{
    string varName;
    ASTNodePtr fromExpr;
    ASTNodePtr toExpr;
    bool isDownto = false;
    ASTNodePtr body;

    ForNode(const string &v, ASTNodePtr f, ASTNodePtr t, bool d, ASTNodePtr b)
        : varName(v), fromExpr(move(f)), toExpr(move(t)),
          isDownto(d), body(move(b)) { dtype = DataType::VOID; }
    string nodeName() const override
    {
        return string("For(") + varName + ", " + (isDownto ? "downto" : "to") + ")";
    }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct CaseNode : ASTNode
{
    ASTNodePtr selector;
    vector<pair<vector<string>, ASTNodePtr>> branches;

    explicit CaseNode(ASTNodePtr s) : selector(move(s)) { dtype = DataType::VOID; }
    string nodeName() const override { return "Case"; }
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct ArrayAccessNode : ASTNode
{
    ASTNodePtr array;
    vector<ASTNodePtr> indices;

    ArrayAccessNode(ASTNodePtr a, vector<ASTNodePtr> i)
        : array(move(a)), indices(move(i)) {}
    string nodeName() const override { return "ArrayAccess"; }
    string toSource() const override;
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};

struct FieldAccessNode : ASTNode
{
    ASTNodePtr record;
    string fieldName;

    FieldAccessNode(ASTNodePtr r, const string &f)
        : record(move(r)), fieldName(f) {}
    string nodeName() const override { return "FieldAccess(" + fieldName + ")"; }
    string toSource() const override;
    void print(ostream &out, const string &prefix, bool isLast, bool isRoot) const override;
};