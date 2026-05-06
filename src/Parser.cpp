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
    node->children.push_back(parseCompoundStatement()); // TODO
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

shared_ptr<ParseNode> Parser::parseRange(shared_ptr<ParseNode> firstConst)
{
    auto node = makeNode("<range>");
    node->children.push_back(firstConst);

    node->children.push_back(expect("period"));
    node->children.push_back(expect("period"));

    node->children.push_back(parseConstant());

    return node;
}

shared_ptr<ParseNode> Parser::parseRecordType()
{
    auto node = makeNode("<record-type>");

    node->children.push_back(expect("recordsy"));
    node->children.push_back(parseFieldList());
    node->children.push_back(expect("endsy"));

    return node;
}


shared_ptr<ParseNode> Parser::parseType()
{
    auto node = makeNode("<type>");

    if (check("arraysy")) {
        node->children.push_back(parseArrayType());
    }
    else if (check("lparent")) {
        node->children.push_back(parseEnumerated());
    }
    else if (check("recordsy")) {
        node->children.push_back(parseRecordType());
    }
    else if (check("ident") && lookahead().type == "period") {
        auto firstConst = makeNode("<constant>");
        firstConst->children.push_back(consume()); // consume ident
        node->children.push_back(parseRange(firstConst));
    }
    else if (check("intcon") || check("realcon") || check("charcon") ||
             check("string") || check("plus") || check("minus"))
    {
        auto firstConst = parseConstant();
        if (check("period")) {
            node->children.push_back(parseRange(firstConst));
        }
        else {
            // Standalone constant (jarang, tapi valid secara grammar)
            node->children.push_back(firstConst);
        }
    }
    else if (check("ident")) {
        node->children.push_back(expect("ident"));
    }
    else
    {
        syntaxError("type (ident, array-type, range, enumerated, or record-type)");
    }

    return node;
}

shared_ptr<ParseNode> Parser::parseIdentifierList()
{
    auto node = makeNode("<identifier-list>");
    node->children.push_back(expect("ident"));

    while (check("comma") && lookahead().type == "ident") {
        node->children.push_back(expect("comma"));
        node->children.push_back(expect("ident"));
    }

    return node;
}

shared_ptr<ParseNode> Parser::parseFieldPart()
{
    auto node = makeNode("<field-part>");

    node->children.push_back(parseIdentifierList());
    node->children.push_back(expect("colon"));
    node->children.push_back(parseType());

    return node;
}

shared_ptr<ParseNode> Parser::parseFieldList()
{
    auto node = makeNode("<field-list>");

    node->children.push_back(parseFieldPart());

    while (check("semicolon") && lookahead().type == "ident")
    {
        node->children.push_back(expect("semicolon"));
        node->children.push_back(parseFieldPart());
    }

    return node;
}

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


shared_ptr<ParseNode> Parser::parseTypeDeclaration()
{
    auto node = makeNode("<type-declaration>");

    node->children.push_back(expect("typesy"));

    do
    {
        node->children.push_back(expect("ident"));
        node->children.push_back(expect("eql"));
        node->children.push_back(parseType());
        node->children.push_back(expect("semicolon"));
    } while (check("ident")); // lanjut selama masih ada ident berikutnya

    return node;
}

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
        node->children.push_back(parseIdentifierList());
        node->children.push_back(expect("colon"));
        node->children.push_back(parseType());
        node->children.push_back(expect("semicolon"));
    }

    return node;
}

shared_ptr<ParseNode> Parser::parseCompoundStatement()
{
    auto node = makeNode("<compound-statement>");

    node->children.push_back(expect("beginsy"));
    node->children.push_back(expect("endsy"));

    return node;
}

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

shared_ptr<ParseNode> Parser::parseIndexList()
{
    auto node = makeNode("<index-list>");

    if (check("intcon") || check("charcon") || check("ident"))
    {
        node->children.push_back(consume());
    }
    else
    {
        syntaxError("intcon, charcon, or ident in index-list");
    }

    while (check("comma"))
    {
        node->children.push_back(expect("comma"));
        node->children.push_back(parseIndexList());
    }

    return node;
}