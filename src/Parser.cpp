#include "Parser.h"
#include <sstream>
using namespace std;

Parser::Parser(const vector<Token> &t) : tokens(t), pos(0) {}

shared_ptr<ParseNode> Parser::parse()
{
    return parseProgram();
}

static const Token EOF_TOKEN{"eof", ""};

const Token &Parser::current()
{
    // Skip komemtar
    while (static_cast<size_t>(pos) < tokens.size() && tokens[pos].type == "comment")
    {
        const_cast<Parser *>(this)->pos++;
    }

    if (static_cast<size_t>(pos) < tokens.size())
    {
        return tokens[pos];
    }

    return EOF_TOKEN;
}

const Token &Parser::lookahead()
{
    // Skip komentar
    int after = pos + 1;
    while (static_cast<size_t>(after) < tokens.size() && tokens[after].type == "comment")
    {
        after++;
    }

    if (static_cast<size_t>(after) < tokens.size())
    {
        return tokens[after];
    }

    return EOF_TOKEN;
}

bool Parser::check(const string &type)
{
    return current().type == type;
}

bool Parser::isAtEnd()
{
    return (static_cast<size_t>(pos) >= tokens.size() || current().type == "eof");
}

void Parser::syntaxError(const string &expected)
{
    const Token &tok = current();
    ostringstream oss;
    oss << "Syntax error: unexpected token '" << tok.type;
    if (!tok.value.empty())
        oss << "(" << tok.value << ")";
    oss << "', expected " << expected;
    throw runtime_error(oss.str());
}

shared_ptr<ParseNode> Parser::consume()
{
    const Token &tok = current();
    auto leaf = makeLeaf(tok.type, tok.value);
    pos++;
    return leaf;
}

shared_ptr<ParseNode> Parser::expect(const string &type)
{
    if (current().type != type)
        syntaxError(type);
    return consume();
}

shared_ptr<ParseNode> Parser::expectVal(const string &type, const string &val)
{
    if (current().type != type || current().value != val)
        syntaxError(type + "(" + val + ")");
    return consume();
}

shared_ptr<ParseNode> Parser::parseProgram()
{
    auto node = makeNode("<program>");
    node->children.push_back(parseProgramHeader());
    node->children.push_back(parseDeclarationPart());
    node->children.push_back(parseCompoundStatement());
    node->children.push_back(expect("period"));

    if (!isAtEnd())
    {
        syntaxError("end of program");
    }

    return node;
}

shared_ptr<ParseNode> Parser::parseProgramHeader()
{
    auto node = makeNode("<program-header>");
    node->children.push_back(expect("programsy"));
    node->children.push_back(expect("ident"));
    node->children.push_back(expect("semicolon"));
    return node;
}
shared_ptr<ParseNode> Parser::parseDeclarationPart()
{
    auto node = makeNode("<declaration-part>");
    while (check("constsy"))
    {
        node->children.push_back(parseConstDeclaration());
    }

    while (check("typesy"))
    {
        node->children.push_back(parseTypeDeclaration());
    }

    while (check("varsy"))
    {
        node->children.push_back(parseVarDeclaration());
    }

    while (check("proceduresy") || check("functionsy"))
    {
        node->children.push_back(parseSubProgramDeclaration());
    }

    return node;
}
shared_ptr<ParseNode> Parser::parseConstDeclaration()
{
    auto node = makeNode("<const-declaration>");
    node->children.push_back(expect("constsy"));
    while (check("ident"))
    {
        node->children.push_back(expect("ident"));
        node->children.push_back(expect("eql"));
        node->children.push_back(parseConstant());
        node->children.push_back(expect("semicolon"));
    }
    return node;
}
shared_ptr<ParseNode> Parser::parseConstant()
{
    auto node = makeNode("<constant>");
    if (check("string") || check("charcon"))
    {
        node->children.push_back(consume());
    }
    else if (check("plus") || check("minus"))
    {
        node->children.push_back(consume());
        if (check("intcon") || check("realcon") || check("ident"))
        {
            node->children.push_back(consume());
        }
        else
        {
            syntaxError("ident, intcon, or realcon adter sign in constant");
        }
    }
    else if (check("intcon") || check("realcon") || check("ident"))
    {
        node->children.push_back(consume());
    }
    else
    {
        syntaxError("constant (charcon, string, intcon, realcon, or signed value)");
    }

    return node;
}

// STUB
shared_ptr<ParseNode> Parser::parseIdentifierList()
{
    auto node = makeNode("<identifier-list>");

    // Stub minimal supaya compile

    return node;
}

// STUB
shared_ptr<ParseNode> Parser::parseArrayType()
{
    auto node = makeNode("<array-type-STUB>");

    // Stub minimal supaya compile

    return node;
}

shared_ptr<ParseNode> Parser::parseSubProgramDeclaration()
{
    auto node = makeNode("<subprogram-declaration>");

    if (check("proceduresy"))
    {
        node->children.push_back(parseProcedureDeclaration());
    }
    else if (check("functionsy"))
    {
        node->children.push_back(parseFunctionDeclaration());
    }
    else
    {
        syntaxError("proceduresy or functionsy");
    }

    return node;
}

shared_ptr<ParseNode> Parser::parseProcedureDeclaration()
{
    auto node = makeNode("<procedure-declaration>");

    node->children.push_back(expect("proceduresy"));
    node->children.push_back(expect("ident"));

    if (check("lparent"))
    {
        node->children.push_back(parseFormalParameterList());
    }

    node->children.push_back(expect("semicolon"));
    node->children.push_back(parseBlock());
    node->children.push_back(expect("semicolon"));

    return node;
}

shared_ptr<ParseNode> Parser::parseFunctionDeclaration()
{
    auto node = makeNode("<function-declaration>");

    node->children.push_back(expect("functionsy"));
    node->children.push_back(expect("ident"));

    // formal-parameter-list opsional
    if (check("lparent"))
    {
        node->children.push_back(parseFormalParameterList());
    }

    node->children.push_back(expect("colon"));
    node->children.push_back(expect("ident"));
    node->children.push_back(expect("semicolon"));

    node->children.push_back(parseBlock());
    node->children.push_back(expect("semicolon"));

    return node;
}

shared_ptr<ParseNode> Parser::parseBlock()
{
    auto node = makeNode("<block>");

    node->children.push_back(parseDeclarationPart());
    node->children.push_back(parseCompoundStatement());

    return node;
}

shared_ptr<ParseNode> Parser::parseFormalParameterList()
{
    auto node = makeNode("<formal-parameter-list>");

    node->children.push_back(expect("lparent"));
    node->children.push_back(parseParameterGroup());

    while (check("semicolon"))
    {
        node->children.push_back(expect("semicolon"));
        node->children.push_back(parseParameterGroup());
    }

    node->children.push_back(expect("rparent"));

    return node;
}

shared_ptr<ParseNode> Parser::parseParameterGroup()
{
    auto node = makeNode("<parameter-group>");

    node->children.push_back(parseIdentifierList());
    node->children.push_back(expect("colon"));

    // Sesuai spek: parameter-group -> identifier-list colon (ident | array-type)
    if (check("ident"))
    {
        node->children.push_back(expect("ident"));
    }
    else if (check("arraysy"))
    {
        node->children.push_back(parseArrayType());
    }
    else
    {
        syntaxError("ident or array-type");
    }

    return node;
}

// STUB FOR TESTING
// Fungsi-fungsi ini sementara saja supaya Parser.cpp bisa di-compile
// sebelum bagian Orang 2, 3, dan 4 selesai.

shared_ptr<ParseNode> Parser::parseTypeDeclaration()
{
    auto node = makeNode("<type-declaration-STUB>");

    // Stub ini tidak memproses type declaration.
    // Jangan dipakai untuk test type dulu.
    return node;
}

shared_ptr<ParseNode> Parser::parseVarDeclaration()
{
    auto node = makeNode("<var-declaration-STUB>");

    // Stub ini tidak memproses var declaration.
    // Jangan dipakai untuk test var dulu.
    return node;
}

shared_ptr<ParseNode> Parser::parseCompoundStatement()
{
    auto node = makeNode("<compound-statement>");

    node->children.push_back(expect("beginsy"));
    node->children.push_back(parseStatementList());
    node->children.push_back(expect("endsy"));

    return node;
}

shared_ptr<ParseNode> Parser::parseStatementList()
{
    auto node = makeNode("<statement-list>");
    if (check("endsy") || check("untilsy") || isAtEnd())
        return node;
    node->children.push_back(parseStatement());
    while (check("semicolon"))
    {
        node->children.push_back(expect("semicolon"));

        if (check("endsy") || check("untilsy") || isAtEnd())
            break;

        node->children.push_back(parseStatement());
    }
    return node;
}

shared_ptr<ParseNode> Parser::parseVariable()
{
    auto node = makeNode("<variable>");
    node->children.push_back(expect("ident"));

    if (check("lbrack") || check("period"))
    {
        auto componentNode = makeNode("<component-variable>");

        componentNode->children.push_back(node);

        while (check("lbrack") || check("period"))
        {

            if (check("lbrack"))
            {
                componentNode->children.push_back(expect("lbrack"));
                componentNode->children.push_back(parseIndexList());
                componentNode->children.push_back(expect("rbrack"));
            }
            else
            {
                componentNode->children.push_back(expect("period"));
                componentNode->children.push_back(expect("ident"));
            }
        }
        return componentNode;
    }
    return node;
}

shared_ptr<ParseNode> Parser::parseIndexList()
{
    auto node = makeNode("<index-list>");

    if (check("intcon") || check("charcon") || check("ident"))
        node->children.push_back(consume());
    else
        syntaxError("intcon, charcon, or ident as index");

    while (check("comma"))
    {
        node->children.push_back(expect("comma"));
        if (check("intcon") || check("charcon") || check("ident"))
            node->children.push_back(consume());
        else
            syntaxError("intcon, charcon, or ident as index");
    }

    return node;
}

shared_ptr<ParseNode> Parser::parseStatement()
{
    auto node = makeNode("<statement>");
    if (check("ifsy"))
    {
        node->children.push_back(parseIfStatement());
    }
    else if (check("casesy"))
    {
        node->children.push_back(parseCaseStatement());
    }
    else if (check("whilesy"))
    {
        node->children.push_back(parseWhileStatement());
    }
    else if (check("repeatsy"))
    {
        node->children.push_back(parseRepeatStatement());
    }
    else if (check("forsy"))
    {
        node->children.push_back(parseForStatement());
    }
    else if (check("ident"))
    {

        auto identLeaf = parseVariable();

        if (check("becomes"))
            node->children.push_back(parseAssignmentStatement(identLeaf));
        else
            node->children.push_back(parseProcedureFunctionCall(identLeaf));
    }

    return node;
}
shared_ptr<ParseNode> Parser::parseAssignmentStatement(shared_ptr<ParseNode> identLeaf)
{
    auto node = makeNode("<assignment-statement>");
    node->children.push_back(identLeaf);
    node->children.push_back(expect("becomes"));
    node->children.push_back(parseExpression());
    return node;
}
shared_ptr<ParseNode> Parser::parseIfStatement()
{
    auto node = makeNode("<if-statement>");
    node->children.push_back(expect("ifsy"));
    node->children.push_back(parseExpression());
    node->children.push_back(expect("thensy"));
    node->children.push_back(parseStatement());
    if (check("elsesy"))
    {
        node->children.push_back(expect("elsesy"));

        node->children.push_back(parseStatement());
    }
    return node;
}
shared_ptr<ParseNode> Parser::parseCaseStatement()
{
    auto node = makeNode("<case-statement>");
    node->children.push_back(expect("casesy"));
    node->children.push_back(parseExpression());
    node->children.push_back(expect("ofsy"));
    node->children.push_back(parseCaseBlock());

    while (check("semicolon") && !isAtEnd())
    {
        node->children.push_back(expect("semicolon"));
        if (check("endsy"))
            break;
        node->children.push_back(parseCaseBlock());
    }

    node->children.push_back(expect("endsy"));
    return node;
}
shared_ptr<ParseNode> Parser::parseCaseBlock()
{
    auto node = makeNode("<case-block>");
    node->children.push_back(parseConstant());
    while (check("comma"))
    {
        node->children.push_back(expect("comma"));

        node->children.push_back(parseConstant());
    }
    node->children.push_back(expect("colon"));
    node->children.push_back(parseStatement());
    return node;
}
shared_ptr<ParseNode> Parser::parseWhileStatement()
{
    auto node = makeNode("<while-statement>");
    node->children.push_back(expect("whilesy"));
    node->children.push_back(parseExpression());
    node->children.push_back(expect("dosy"));
    node->children.push_back(parseStatement());
    return node;
}
shared_ptr<ParseNode> Parser::parseRepeatStatement()
{
    auto node = makeNode("<repeat-statement>");
    node->children.push_back(expect("repeatsy"));
    node->children.push_back(parseStatementList());
    node->children.push_back(expect("untilsy"));
    node->children.push_back(parseExpression());
    return node;
}
shared_ptr<ParseNode> Parser::parseForStatement()
{
    auto node = makeNode("<for-statement>");
    node->children.push_back(expect("forsy"));
    node->children.push_back(expect("ident")); 
    node->children.push_back(expect("becomes"));
    node->children.push_back(parseExpression());

    if (check("tosy"))
        node->children.push_back(expect("tosy"));
    else if (check("downtosy"))
        node->children.push_back(expect("downtosy"));
    else
        syntaxError("tosy or downtosy");

    node->children.push_back(parseExpression());
    node->children.push_back(expect("dosy"));
    node->children.push_back(parseStatement());
    return node;
}
shared_ptr<ParseNode> Parser::parseProcedureFunctionCall(shared_ptr<ParseNode> identLeaf)
{
    auto node = makeNode("<procedure/function-call>");
    node->children.push_back(identLeaf);
    if (check("lparent"))
    {
        node->children.push_back(expect("lparent"));
        if (!check("rparent"))
        {
            node->children.push_back(parseParameterList());
        }
        node->children.push_back(expect("rparent"));
    }
    return node;
}
shared_ptr<ParseNode> Parser::parseParameterList()
{
    auto node = makeNode("<parameter-list>");
    node->children.push_back(parseExpression());
    while (check("comma"))
    {
        node->children.push_back(expect("comma"));
        node->children.push_back(parseExpression());
    }
    return node;
}

shared_ptr<ParseNode> Parser::parseExpression()
{
    auto node = makeNode("<expression-STUB>");

    // Consume token sampai ketemu delimiter
    while (!isAtEnd() && !check("thensy") && !check("dosy") && !check("untilsy") && !check("ofsy") && !check("endsy") && !check("semicolon") && !check("period") && !check("rparent") && !check("comma") && !check("elsesy") && !check("tosy") && !check("downtosy") && !check("rbrack") )
    {
        node->children.push_back(consume());
    }

    return node;
}

// Expression

/**
 * simple-expression (relational-operator + simple-expression)?
 * 
 * Must contain simple-expression
 * Check trailing relational-operator + simple-expression 
 */
shared_ptr<ParseNode> Parser::parseExpression()
{
    auto node = makeNode("<expression>");

    node->children.push_back(parseSimpleExpression());

    if (check("eql") || check("neq") || check("gtr") || check("geq") || check("lss") || check("leq"))
    {
        node->children.push_back(parseRelationalOperator());
        node->children.push_back(parseSimpleExpression());
    }

    return node;
}

/**
 * (plus | minus)? term (additive-operator + term)*
 * 
 * Check any + or - sign
 * Must contain a term
 * Check if trailed by other additive-operator + term
 */
shared_ptr<ParseNode> Parser::parseSimpleExpression()
{
    auto node = makeNode("<simple-expression>");

    if (check("plus") || check("minus"))
    {
        node->children.push_back(consume());
    }

    node->children.push_back(parseTerm());
    
    while (check("plus") || check("minus") || check("orsy"))
    {
        node->children.push_back(parseAdditiveOperator());
        node->children.push_back(parseTerm());
    }

    return node;
}

/**
 * factor (multiplicative-operator + factor)*
 * 
 * Must contain a factor
 * Check if trailed by multiplicative-operator + factor
 */
shared_ptr<ParseNode> Parser::parseTerm()
{
    auto node = makeNode("<term>");

    node->children.push_back(parseFactor());

    while (check("times") || check("idiv") || check("rdiv") || check("imod") || check("andsy"))
    {
        node->children.push_back(parseMultiplicativeOperator());
        node->children.push_back(parseFactor());
    }

    return node;
}

/**
 * ident | intcon | realcon | charcon | string | 
 * (lparent + expression + rparent) | 
 * (notsy + factor) | 
 * procedure/function-call | 
 * variable
 */
shared_ptr<ParseNode> Parser::parseFactor()
{
    auto node = makeNode("<factor>");

    if (check("ident") || check("intcon") || check("realcon") || check("charcon") || check("string"))
        node->children.push_back(consume());
    else if (check("lparent"))
    {
        node->children.push_back(consume());
        node->children.push_back(parseExpression());
        node->children.push_back(expect("rparent"));
    }
    else if (check("notsy"))
    {
        node->children.push_back(consume());
        node->children.push_back(parseFactor());
    }
    else // cek apakah dia variable?
        node->children.push_back(parseVarDeclaration()); // Asumsi pengecekan variable (other opt using ident)

    return node;
}

/**
 * eql | neq | gtr | geq | lss | leq
 */
shared_ptr<ParseNode> Parser::parseRelationalOperator()
{
    auto node = makeNode("<relational-operator>");

    if (check("eql") || check("neq") || check("gtr") || check("geq") || check("lss") || check("leq"))
        node->children.push_back(consume());
    else 
        syntaxError("relational-operator");
    return node;
}

/**
 * plus | minus | orsy
 */
shared_ptr<ParseNode> Parser::parseAdditiveOperator()
{
    auto node = makeNode("<additive-operator>");

    if (check("plus") || check("minus") || check("orsy")) 
        node->children.push_back(consume());
    else
        syntaxError("additive-operator");
    return node;
}

/**
 * times | rdiv | idiv | imod | andsy
 */
shared_ptr<ParseNode> Parser::parseMultiplicativeOperator()
{
    auto node = makeNode("<multiplicative-operator>");

    if(check("times") || check("idiv") || check("rdiv") || check("imod") || check("andsy"))
        node->children.push_back(consume());
    else
        syntaxError("multiplicative-operator");
    return node;
}
