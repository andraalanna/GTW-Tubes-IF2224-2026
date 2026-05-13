#pragma once->Orang 3(file ini support)

#include "lexer.h"
#include "symbolTable.hpp"
#include "TypeSystem.h"
#include "ErrorHandler.h"
#include <memory>
#include <string>
#include <vector>

struct ASTNode
{
    std::string kind;
    std::string value;

    DataType dtype;
    int tabIndex;
    int lexLevel;

    std::vector<std::shared_ptr<ASTNode>> children;

    ASTNode(const std::string &k, const std::string &v = "")
        : kind(k), value(v),
          dtype(DataType::UNKNOWN), tabIndex(-1), lexLevel(0) {}
};

using ASTNodePtr = std::shared_ptr<ASTNode>;

inline ASTNodePtr makeAST(const std::string &kind, const std::string &val = "")
{
    return std::make_shared<ASTNode>(kind, val);
}
class SemanticAnalyzer
{
public:
    explicit SemanticAnalyzer();

    ASTNodePtr analyze(std::shared_ptr<ParseNode> parseRoot);

    const SymbolTable &getSymbolTable() const { return symTable; }
    SymbolTable &getSymbolTable() { return symTable; }

    // --- Orang 4: Deklarasi ---
    ASTNodePtr visitProgram(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitProgramHeader(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitDeclarationPart(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitConstDeclaration(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitTypeDeclaration(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitVarDeclaration(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitProcDeclaration(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitFuncDeclaration(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitBlock(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitFormalParamList(std::shared_ptr<ParseNode> n);

    // --- Orang 5: Statements ---
    ASTNodePtr visitCompoundStatement(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitStatementList(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitStatement(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitAssignStatement(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitIfStatement(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitWhileStatement(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitRepeatStatement(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitForStatement(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitCaseStatement(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitProcFuncCall(std::shared_ptr<ParseNode> n);

    ASTNodePtr visitExpression(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitSimpleExpression(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitTerm(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitFactor(std::shared_ptr<ParseNode> n);
    ASTNodePtr visitVariable(std::shared_ptr<ParseNode> n);

    // Resolve DataType dari node <type> di parse tree
    // (ident -> lookup tab, array-type -> atab, dll)
    DataType resolveType(std::shared_ptr<ParseNode> typeNode, int &outRef);

    // Print Decorated AST ke ostream
    void printAST(ASTNodePtr root, std::ostream &out,
                  const std::string &prefix = "", bool isLast = true, bool isRoot = true) const;

private:
    SymbolTable symTable;

    // Helper: cari child ParseNode berdasarkan type string
    std::shared_ptr<ParseNode> findChild(std::shared_ptr<ParseNode> n,
                                         const std::string &type) const;

    // Helper: kumpulkan semua child dengan type tertentu
    std::vector<std::shared_ptr<ParseNode>> findChildren(
        std::shared_ptr<ParseNode> n, const std::string &type) const;

    // Helper: buat SourceLocation dari ParseNode leaf
    SourceLocation locOf(std::shared_ptr<ParseNode> n) const;

    // Helper: hitung ukuran tipe dalam "unit memori" (1 per scalar, size untuk array)
    int sizeOf(DataType t, int ref) const;
};