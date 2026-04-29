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

shared_ptr<ParseNode> Parser::parseSubProgramDeclaration()
{
    auto node = makeNode("<subprogram-declaration-STUB>");

    // Stub ini tidak memproses procedure/function declaration.
    // Jangan dipakai untuk test subprogram dulu.
    return node;
}

shared_ptr<ParseNode> Parser::parseCompoundStatement()
{
    auto node = makeNode("<compound-statement>");

    node->children.push_back(expect("beginsy"));
    node->children.push_back(expect("endsy"));

    return node;
}