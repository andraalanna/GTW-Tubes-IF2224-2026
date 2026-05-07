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
    oss << "Syntax error at line " << tok.line << ", col " << tok.col << ": unexpected token '" << tok.type;
    if (!tok.value.empty())
        oss << "(" << tok.value << ")";
    oss << "', expected " << expected;
    throw SyntaxError(oss.str());
}

void Parser::synchronize()
{
    while (!isAtEnd())
    {
        if (check("semicolon"))
        {
            pos++;
            return;
        }

        if (check("constsy") ||
            check("typesy") ||
            check("varsy") ||
            check("proceduresy") ||
            check("functionsy") ||
            check("beginsy") ||
            check("endsy") ||
            check("ifsy") ||
            check("whilesy") ||
            check("repeatsy") ||
            check("forsy") ||
            check("casesy"))
        {
            return;
        }

        pos++;
    }
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

/**
 * program → program-header + declaration-part + compound-statement + period
 */
shared_ptr<ParseNode> Parser::parseProgram()
{
    auto node = makeNode("<program>");

    try
    {
        node->children.push_back(parseProgramHeader());
    }
    catch (const SyntaxError &e)
    {
        errors.push_back(e.what());
        synchronize();
    }

    try
    {
        node->children.push_back(parseDeclarationPart());
    }
    catch (const SyntaxError &e)
    {
        errors.push_back(e.what());
        synchronize();
    }

    try
    {
        node->children.push_back(parseCompoundStatement());
    }
    catch (const SyntaxError &e)
    {
        errors.push_back(e.what());
        synchronize();
    }

    try
    {
        node->children.push_back(expect("period"));
    }
    catch (const SyntaxError &e)
    {
        errors.push_back(e.what());
    }

    if (!isAtEnd())
    {
        const Token &tok = current();
        ostringstream oss;
        oss << "Syntax error at line " << tok.line << ", col " << tok.col
            << ": unexpected token '" << tok.type
            << "', expected end of program";
        errors.push_back(oss.str());
    }

    return node;
}

/**
 * programsy + ident + semicolon
 */
shared_ptr<ParseNode> Parser::parseProgramHeader()
{
    auto node = makeNode("<program-header>");
    node->children.push_back(expect("programsy"));
    node->children.push_back(expect("ident"));
    node->children.push_back(expect("semicolon"));
    return node;
}

/**
 * (const-declaration)* + (type-declaration)* + (var-declaration)* + (subprogram-declaration)*
 */
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
        try
        {
            node->children.push_back(parseSubProgramDeclaration());
        }
        catch (const SyntaxError &e)
        {
            errors.push_back(e.what());
            synchronizeSubprogram();
        }
    }
    return node;
}

/**
 * constsy + (ident + eql + constant + semicolon)+
 */
shared_ptr<ParseNode> Parser::parseConstDeclaration()
{
    auto node = makeNode("<const-declaration>");
    node->children.push_back(expect("constsy"));
    while (check("ident"))
    {
        try
        {
            node->children.push_back(expect("ident"));
            node->children.push_back(expect("eql"));
            node->children.push_back(parseConstant());
            node->children.push_back(expect("semicolon"));
        }
        catch (const SyntaxError &e)
        {
            errors.push_back(e.what());
            synchronize();
        }
    }
    return node;
}

/**
 * charcon | string | [(plus | minus)? + (ident | intcon | realcon)]
 */
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
            syntaxError("ident, intcon, or realcon after sign in constant");
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

/**
 * typesy + (ident + eql + type + semicolon)+
 */
shared_ptr<ParseNode> Parser::parseTypeDeclaration()
{
    auto node = makeNode("<type-declaration>");

    node->children.push_back(expect("typesy"));
    if (!check("ident"))
    {
        syntaxError("ident (expected at least one type declaration after 'type')");
    }

    while (check("ident")) // lanjut selama masih ada ident berikutnya
    {
        try
        {
            node->children.push_back(expect("ident"));
            node->children.push_back(expect("eql"));
            node->children.push_back(parseType());
            node->children.push_back(expect("semicolon"));
        }
        catch (const SyntaxError &e)
        {
            errors.push_back(e.what());
            synchronize();
        }
    }

    return node;
}

/**
 * varsy + (identifier-list + colon + type + semicolon)+
 */
shared_ptr<ParseNode> Parser::parseVarDeclaration()
{
    auto node = makeNode("<var-declaration>");

    node->children.push_back(expect("varsy"));

    if (!check("ident"))
    {
        syntaxError("ident (expected at least one variable declaration after 'var')");
    }

    while (check("ident"))
    {
        try
        {
            node->children.push_back(parseIdentifierList());
            node->children.push_back(expect("colon"));
            node->children.push_back(parseType());
            node->children.push_back(expect("semicolon"));
        }
        catch (const SyntaxError &e)
        {
            errors.push_back(e.what());
            synchronize();
        }
    }

    return node;
}

/**
 * ident (comma + ident)*
 */
shared_ptr<ParseNode> Parser::parseIdentifierList()
{
    auto node = makeNode("<identifier-list>");
    node->children.push_back(expect("ident"));

    while (check("comma") && lookahead().type == "ident")
    {
        node->children.push_back(expect("comma"));
        node->children.push_back(expect("ident"));
    }

    return node;
}

/**
 * ident | array-type | range | enumerated | record-type
 */
shared_ptr<ParseNode> Parser::parseType()
{
    auto node = makeNode("<type>");

    if (check("arraysy"))
    {
        node->children.push_back(parseArrayType());
    }
    else if (check("lparent"))
    {
        node->children.push_back(parseEnumerated());
    }
    else if (check("recordsy"))
    {
        node->children.push_back(parseRecordType());
    }
    else if (check("ident") && lookahead().type == "period")
    {
        auto firstConst = makeNode("<constant>");
        firstConst->children.push_back(consume()); // consume ident
        node->children.push_back(parseRange(firstConst));
    }
    else if (check("intcon") || check("charcon") ||
            check("plus") || check("minus"))
    {
        auto firstConst = parseConstant();
        if (check("period"))
        {
            node->children.push_back(parseRange(firstConst));
        }
        else
        {
            syntaxError("'..' expected after constant in range type");
        }
    }
    else if (check("ident"))
    {
        node->children.push_back(expect("ident"));
    }
    else
    {
        syntaxError("type (ident, array-type, range, enumerated, or record-type)");
    }

    return node;
}

/**
 * arraysy + lbrack + (range | ident) + rbrack + ofsy + type
 */
shared_ptr<ParseNode> Parser::parseArrayType()
{
    auto node = makeNode("<array-type>");

    node->children.push_back(expect("arraysy"));
    node->children.push_back(expect("lbrack"));

    if (check("ident") && lookahead().type == "period")
    {
        // Range diawali ident
        auto firstConst = makeNode("<constant>");
        firstConst->children.push_back(consume()); // consume ident
        node->children.push_back(parseRange(firstConst));
    }
    else if (check("intcon") || check("charcon") || check("realcon") ||
             check("plus") || check("minus"))
    {
        // Range diawali intcon/charcon/etc
        auto firstConst = parseConstant();
        node->children.push_back(parseRange(firstConst));
    }
    else if (check("ident"))
    {
        node->children.push_back(expect("ident"));
    }
    else
    {
        syntaxError("range or ident as array index type");
    }

    node->children.push_back(expect("rbrack"));
    node->children.push_back(expect("ofsy"));

    // Element type, bisa nested array juga
    node->children.push_back(parseType());

    return node;
}

/**
 * constant + period + period + constant
 */
shared_ptr<ParseNode> Parser::parseRange(shared_ptr<ParseNode> firstConst)
{
    auto node = makeNode("<range>");
    node->children.push_back(firstConst);

    node->children.push_back(expect("period"));
    node->children.push_back(expect("period"));

    node->children.push_back(parseConstant());

    return node;
}

/**
 * lparent + ident + (comma + ident)* + rparent
 */
shared_ptr<ParseNode> Parser::parseEnumerated()
{
    auto node = makeNode("<enumerated>");

    node->children.push_back(expect("lparent"));
    node->children.push_back(expect("ident"));

    while (check("comma"))
    {
        node->children.push_back(expect("comma"));
        node->children.push_back(expect("ident"));
    }

    node->children.push_back(expect("rparent"));

    return node;
}

/**
 * recordsy + field-list + endsy
 */
shared_ptr<ParseNode> Parser::parseRecordType()
{
    auto node = makeNode("<record-type>");

    node->children.push_back(expect("recordsy"));
    node->children.push_back(parseFieldList());
    node->children.push_back(expect("endsy"));

    return node;
}

/**
 * field-part + (semicolon + field-part)*
 */
shared_ptr<ParseNode> Parser::parseFieldList()
{
    auto node = makeNode("<field-list>");

    node->children.push_back(parseFieldPart());

    while (check("semicolon") && lookahead().type != "endsy")
    {
        node->children.push_back(expect("semicolon"));
        node->children.push_back(parseFieldPart());
    }

    return node;
}

/**
 * identifier-list + colon + type
 */
shared_ptr<ParseNode> Parser::parseFieldPart()
{
    auto node = makeNode("<field-part>");

    node->children.push_back(parseIdentifierList());
    node->children.push_back(expect("colon"));
    node->children.push_back(parseType());

    return node;
}

void Parser::synchronizeSubprogram()
{
    while (!isAtEnd())
    {
        if (check("proceduresy") || check("functionsy"))
        {
            return;
        }

        if (check("endsy") && lookahead().type == "semicolon")
        {
            pos++;
            pos++;
            return;
        }

        pos++;
    }
}

/**
 * procedure-declaration | function-declaration
 */
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
        syntaxError("proceduresy or functionsy after subprogram declaration.");
    }

    return node;
}

/**
 * proceduresy + ident + (formal-parameter-list)? + semicolon + block + semicolon
 */
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

/**
 * functionsy + ident + (formal-parameter-list)? + colon + ident + semicolon + block + semicolon
 */
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

/**
 * declaration-part + compound-statement
 */
shared_ptr<ParseNode> Parser::parseBlock()
{
    auto node = makeNode("<block>");

    node->children.push_back(parseDeclarationPart());
    node->children.push_back(parseCompoundStatement());

    return node;
}

/**
 * lparent + parameter-group + (semicolon + parameter-group)* + rparent
 */
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

/**
 * identifier-list + colon + (ident | array-type)
 */
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

/**
 * beginsy + statement-list + endsy
 */
shared_ptr<ParseNode> Parser::parseCompoundStatement()
{
    auto node = makeNode("<compound-statement>");

    node->children.push_back(expect("beginsy"));
    node->children.push_back(parseStatementList());
    node->children.push_back(expect("endsy"));

    return node;
}

/**
 * statement (semicolon + statement)*
 */

// harusnya grammarnya statement (semicolon + statement)*, izin perbaikin dikit yak
// - yavie
shared_ptr<ParseNode> Parser::parseStatementList()
{
    auto node = makeNode("<statement-list>");
    try
    {
        node->children.push_back(parseStatement());
    }
    catch (const SyntaxError &e)
    {
        errors.push_back(e.what());
        synchronize();
    }

    while (check("semicolon"))
    {
        try
        {
            node->children.push_back(expect("semicolon"));
            node->children.push_back(parseStatement());
        }
        catch (const SyntaxError &e)
        {
            errors.push_back(e.what());
            synchronize();
        }
    }
    return node;
}

/**
 * (assignment-statement | if-statement | case-statement | while-statement | repeat-statement | for-statement | procedure/function-call )?
 */
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

shared_ptr<ParseNode> Parser::parseComponentVariable()
{
    auto node = makeNode("<component-variable>");

    if (check("lbrack"))
    {
        node->children.push_back(expect("lbrack"));
        node->children.push_back(parseIndexList());
        node->children.push_back(expect("rbrack"));
    }
    else if (check("period"))
    {
        node->children.push_back(expect("period"));
        node->children.push_back(expect("ident"));
    }
    else
    {
        syntaxError("'[' or '.' in component-variable");
    }

    return node;
}

/**
 * ident + (component-variable)*
 */
shared_ptr<ParseNode> Parser::parseVariable()
{
    auto node = makeNode("<variable>");
    node->children.push_back(expect("ident"));

    while (check("lbrack") || check("period"))
    {
        node->children.push_back(parseComponentVariable());
    }
    return node;
}

/**
 * ( intcon | charcon | ident ) + ( comma + index-list )*
 */
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

/**
 * variable + becomes + expression
 */
shared_ptr<ParseNode> Parser::parseAssignmentStatement(shared_ptr<ParseNode> identLeaf)
{
    auto node = makeNode("<assignment-statement>");
    node->children.push_back(identLeaf);
    node->children.push_back(expect("becomes"));
    node->children.push_back(parseExpression());
    return node;
}

/**
 * ifsy + expression + thensy + statement + (elsy + statement)?
 */
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

/**
 * casesy + expression + ofsy + case-block + endsy
 */
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

/**
 * constant + (comma + constant)* + colon + statement + (semicolon + case-block?)*
 */
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

/**
 * whilesy + expression + dosy + statement
 */
shared_ptr<ParseNode> Parser::parseWhileStatement()
{
    auto node = makeNode("<while-statement>");
    node->children.push_back(expect("whilesy"));
    node->children.push_back(parseExpression());
    node->children.push_back(expect("dosy"));
    node->children.push_back(parseStatement());
    return node;
}

/**
 * repeatsy + statement-list + untilsy + expression
 */
shared_ptr<ParseNode> Parser::parseRepeatStatement()
{
    auto node = makeNode("<repeat-statement>");
    node->children.push_back(expect("repeatsy"));
    node->children.push_back(parseStatementList());
    node->children.push_back(expect("untilsy"));
    node->children.push_back(parseExpression());
    return node;
}

/**
 * forsy + ident + becomes + expression + ( tosy | downtosy) + expression + dosy + statement
 */
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

/**
 * ident + (lparent + parameter-list? + rparent)
 */
shared_ptr<ParseNode> Parser::parseProcedureFunctionCall(shared_ptr<ParseNode> identLeaf)
{
    auto node = makeNode("<procedure/function-call>");
    node->children.push_back(identLeaf);

    node->children.push_back(expect("lparent"));
    node->children.push_back(parseParameterList());
    node->children.push_back(expect("rparent"));

    return node;
}

/**
 * expression (comma + expression)*
 */
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

    if (check("intcon") || check("realcon") || check("charcon") || check("string"))
        node->children.push_back(consume());
    else if (check("ident"))
    {
        auto identLeaf = consume();
        if (check("lbrack") || check("period"))
        {
            auto varNode = makeNode("<variable>");
            varNode->children.push_back(identLeaf);
            while (check("lbrack") || check("period"))
                varNode->children.push_back(parseComponentVariable());
            node->children.push_back(varNode);
        }
        else if (check("lparent"))
        {
            node->children.push_back(parseProcedureFunctionCall(identLeaf));
        }
        else
        {
            auto varNode = makeNode("<variable>");
            varNode->children.push_back(identLeaf);
            node->children.push_back(varNode);
        }
    }
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
    else                                           // cek apakah dia variable? or function-call
        node->children.push_back(parseVariable()); // Asumsi pengecekan variable (other opt using ident)

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

    if (check("times") || check("idiv") || check("rdiv") || check("imod") || check("andsy"))
        node->children.push_back(consume());
    else
        syntaxError("multiplicative-operator");
    return node;
}