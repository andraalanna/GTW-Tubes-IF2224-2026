#pragma once
#include "lexer.h"
#include "ASTNode.h"    
#include "symbolTable.hpp"
#include <memory>
#include <string>
#include <vector>
#include <map>

using namespace std;

/**
 * ASTBuilder
 * Traverses the concrete ParseNode tree produced by Parser and builds
 * the typed ASTNode tree expected by SemanticAnalyzer.
 *
 * Entry point: build(parseRoot) → ASTNodePtr (ProgramNode)
 */
class ASTBuilder
{
public:
    ASTBuilder(SymbolTable &st);
    ASTNodePtr build(shared_ptr<ParseNode> root);

private:
    // Symbol Table
    SymbolTable &st;

    // Program/declarations
    ASTNodePtr buildProgram(shared_ptr<ParseNode> n);
    void       buildDeclarationPart(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out);
    void       buildConstDeclaration(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out);
    void       buildTypeDeclaration(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out);
    void       buildVarDeclaration(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out);
    void       buildSubprogramDeclaration(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out);
    ASTNodePtr buildProcedureDeclaration(shared_ptr<ParseNode> n);
    ASTNodePtr buildFunctionDeclaration(shared_ptr<ParseNode> n);
    ASTNodePtr buildBlock(shared_ptr<ParseNode> n, vector<ASTNodePtr> &declsOut);

    // Parameters
    void buildFormalParameterList(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out);
    void buildParameterGroup(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out);

    // Statements 
    ASTNodePtr buildCompoundStatement(shared_ptr<ParseNode> n);
    ASTNodePtr buildStatement(shared_ptr<ParseNode> n);
    ASTNodePtr buildAssignmentStatement(shared_ptr<ParseNode> n);
    ASTNodePtr buildIfStatement(shared_ptr<ParseNode> n);
    ASTNodePtr buildWhileStatement(shared_ptr<ParseNode> n);
    ASTNodePtr buildRepeatStatement(shared_ptr<ParseNode> n);
    ASTNodePtr buildForStatement(shared_ptr<ParseNode> n);
    ASTNodePtr buildCaseStatement(shared_ptr<ParseNode> n);
    ASTNodePtr buildProcFuncCall(shared_ptr<ParseNode> n);

    // Expressions
    ASTNodePtr buildExpression(shared_ptr<ParseNode> n);
    ASTNodePtr buildSimpleExpression(shared_ptr<ParseNode> n);
    ASTNodePtr buildTerm(shared_ptr<ParseNode> n);
    ASTNodePtr buildFactor(shared_ptr<ParseNode> n);
    ASTNodePtr buildVariable(shared_ptr<ParseNode> n);

    
    //  Type helpers 
    
    // Resolve a <type> ParseNode to (DataType, atab/btab ref)
    struct TypeInfo { DataType type; int ref; };
    TypeInfo resolveType(shared_ptr<ParseNode> typeNode);
    TypeInfo resolveArrayType(shared_ptr<ParseNode> arrayNode);
    TypeInfo resolveRangeAsAtab(shared_ptr<ParseNode> rangeNode);

    // Parse a <constant> ParseNode to its string value + DataType
    struct ConstInfo { DataType type; string value; };
    ConstInfo resolveConstant(shared_ptr<ParseNode> constNode);

    
    // Utility
    
    // Find first child whose type == tag
    static shared_ptr<ParseNode> child(shared_ptr<ParseNode> n, const string &tag);
    // Find all children with type == tag
    static vector<shared_ptr<ParseNode>> children(shared_ptr<ParseNode> n, const string &tag);
    // Collect all ident leaves from an <identifier-list>
    static vector<string> collectIdents(shared_ptr<ParseNode> identListNode);

    map<string, TypeInfo> declaredTypes;
};
