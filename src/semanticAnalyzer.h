#pragma once

#include "ASTNode.h"
#include "symbolTable.hpp"
#include "TypeSystem.h"
#include "ErrorHandler.h"
#include <memory>
#include <string>


class SemanticAnalyzer
{
public:
    SymbolTable &st;

    explicit SemanticAnalyzer(SymbolTable &symbolTable);
    void analyze(ASTNodePtr root);

    void visitProgram(ProgramNode *node);
    void visitVarDecl(VarDeclNode *node);
    void visitConstDecl(ConstDeclNode *node);
    void visitTypeDecl(TypeDeclNode *node);
    void visitProcDecl(ProcDeclNode *node);
    void visitFuncDecl(FuncDeclNode *node);

    void visitCompoundStmt(CompoundStmtNode *node);
    void visitStatement(ASTNode *node);

    DataType resolveTypeName(const std::string &typeName, int &outRef);

private:
    int elementSize(DataType t, int ref = 0) const;
};