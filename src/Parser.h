#pragma once
#include "lexer.h"
#include <stdexcept>
#include <iostream>
using namespace std;

class Parser {
public:
    Parser(const vector<Token>& tokens);

    // Entry point, kembalikan root ParseNode "<program>"
    shared_ptr<ParseNode> parse();

    shared_ptr<ParseNode> parseProgram();
    shared_ptr<ParseNode> parseProgramHeader();
    shared_ptr<ParseNode> parseDeclarationPart();
    shared_ptr<ParseNode> parseConstDeclaration();
    shared_ptr<ParseNode> parseConstant();

    shared_ptr<ParseNode> parseTypeDeclaration();
    shared_ptr<ParseNode> parseVarDeclaration();
    shared_ptr<ParseNode> parseIdentifierList();
    shared_ptr<ParseNode> parseType();
    shared_ptr<ParseNode> parseArrayType();
    shared_ptr<ParseNode> parseRange(shared_ptr<ParseNode> firstExpr);
    shared_ptr<ParseNode> parseEnumerated();
    shared_ptr<ParseNode> parseRecordType();
    shared_ptr<ParseNode> parseFieldList();
    shared_ptr<ParseNode> parseFieldPart();
    shared_ptr<ParseNode> parseVariable();
    shared_ptr<ParseNode> parseComponentVariable();
    shared_ptr<ParseNode> parseIndexList();


    shared_ptr<ParseNode> parseSubProgramDeclaration();
    shared_ptr<ParseNode> parseProcedureDeclaration();
    shared_ptr<ParseNode> parseFunctionDeclaration();
    shared_ptr<ParseNode> parseBlock();
    shared_ptr<ParseNode> parseFormalParameterList();
    shared_ptr<ParseNode> parseParameterGroup();

    shared_ptr<ParseNode> parseCompoundStatement();
    shared_ptr<ParseNode> parseStatementList();
    shared_ptr<ParseNode> parseStatement();
    shared_ptr<ParseNode> parseAssignmentStatement(shared_ptr<ParseNode> identLeaf);
    shared_ptr<ParseNode> parseIfStatement();
    shared_ptr<ParseNode> parseCaseStatement();
    shared_ptr<ParseNode> parseCaseBlock();
    shared_ptr<ParseNode> parseWhileStatement();
    shared_ptr<ParseNode> parseRepeatStatement();
    shared_ptr<ParseNode> parseForStatement();
    shared_ptr<ParseNode> parseProcedureFunctionCall(shared_ptr<ParseNode> identLeaf);
    shared_ptr<ParseNode> parseParameterList();

    shared_ptr<ParseNode> parseExpression();
    shared_ptr<ParseNode> parseSimpleExpression();
    shared_ptr<ParseNode> parseTerm();
    shared_ptr<ParseNode> parseFactor();
    shared_ptr<ParseNode> parseRelationalOperator();
    shared_ptr<ParseNode> parseAdditiveOperator();
    shared_ptr<ParseNode> parseMultiplicativeOperator();

private:
    vector<Token> tokens;
    int pos;  

    // Lihat token sekarang
    const Token& current();

    // Lihat token selanjutnya, tanpa lewatin yang sekarang
    const Token& lookahead();

    // Cek apakah token saat ini bertipe 'type'
    bool check(const string& type);

    // Konsumsi token saat ini dan kembalikan leaf ParseNode-nya;
    // jika tipe tidak sesuai → lempar syntax error.
    shared_ptr<ParseNode> expect(const string& type);

    // Seperti expect tapi juga cek value (untuk token ganda seperti "eql")
    shared_ptr<ParseNode> expectVal(const string& type, const string& val);

    // Konsumsi token saat ini dan kembalikan leaf-nya (tanpa cek tipe)
    shared_ptr<ParseNode> consume();

    // true kalau udah selesai, false kalau belum.
    bool isAtEnd();

    // Format pesan error syntax
    [[noreturn]] void syntaxError(const string& expected);
};