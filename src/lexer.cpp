#include "lexer.h"
#include <cctype>
#include <iostream>
#include <map>
#include <algorithm>
using namespace std;

Lexer::Lexer(string src)
{
    source = src;
    pos = 0;
    currentState = S0;
};

char Lexer::peek()
{
    if (static_cast<unsigned long int>(pos) >= source.size())
        return '\0';
    return source[pos];
};

char Lexer::advance()
{
    if (static_cast<unsigned long int>(pos) >= source.size())
        return '\0';
    return source[pos++];
};

Token Lexer::readNumber()
{
    string val = "";
    if (currentState == S_MINUS)
    {
        val += '-';
    }

    currentState = S_INT;

    while (isdigit(peek()))
    {
        val += advance();
    }

    if (peek() == '.')
    {
        if (isdigit(source[pos + 1]))
        {
            currentState = S_REAL_DOT;
            val += advance();

            if (!isdigit(peek()))
            {
                currentState = S_ERROR;
                return Token{"unknown", val};
            }
            while (isdigit(peek()))
            {
                val += advance();
            }
            currentState = S_REAL;
            return Token({"realcon", val});
        }
        return Token({"intcon", val});
    }

    if (!isalnum(peek()))
        return Token({"intcon", val});

    while (isalnum(peek()))
    {
        val += advance();
    }
    return Token({"unknown", val});
}

Token Lexer::readComment(char ch)
{
    string val = "";
    if (ch == '{')
    {
        advance();
        while (peek() != '}' && peek() != '\0')
        {
            val += peek();

            advance();
        }
        if (peek() == '\0')
        {
            currentState = S_ERROR;
            return Token{"unknown", "unterminated comment"};
        }
        currentState = S_CMT1;
        advance();
        return Token{"comment", val};
    }
    else
    {
        advance();
        while (peek() != '\0')
        {

            if (peek() == '*')
            {
                currentState = S_CMT2_STAR;
                advance();
                if (peek() == ')')
                {
                    currentState = S_CMT2;
                    advance();
                    return Token{"comment", val};
                }
                val += '*';
                if (peek() != '\0')
                {
                    val += peek();
                    advance();
                }
            }
            else
            {
                val += peek();
                advance();
            }
        }
        currentState = S_ERROR;
        return Token{"unknown", "unterminated comment"};
    }
};
Token Lexer::readString()
{
    string val = "";
    advance();
    while (true)
    {
        char ch = peek();
        if (ch == '\0' || ch == '\n')
        {
            return Token{"unknown", "unterminated string"};
        }

        if (ch == '\'')
        {
            advance();
            if (peek() == '\'')
            {
                advance();
                val += '\'';
            }
            else
            {
                if (val.empty())
                {
                    currentState = S_QUOTE;
                    return Token{"string", "''"};
                }
                if (val.size() == 1)
                {
                    currentState = S_CHAR;
                    return Token{"charcon", "'" + val + "'"};
                }
                currentState = S_QUOTE;
                return Token{"string", "'" + val + "'"};
            }
        }
        else
        {
            val += peek();
            advance();
        }
    }
};

Token Lexer::readOperator(char ch)
{
    string val(1, ch);

    switch (ch)
    {
    case '+':
        currentState = S_PLUS;
        return Token{"plus", val};
    case '-':
        if (isdigit(peek()) && (currentState != S_INT && currentState != S_REAL))
        {
            currentState = S_MINUS;

            return readNumber();
        }
        currentState = S_MINUS;

        return Token{"minus", val};
    case '*':
        currentState = S_TIMES;
        return Token{"times", val};
    case '/':
        currentState = S_RDIV;
        return Token{"rdiv", val};
    case '=':
        currentState = S_EQL1;
        if (peek() == '=')
        {
            val += advance(); // Consume '=' yang kedua
            currentState = S_EQL2;
            return Token{"eql", val};
        }
        currentState = S_ERROR;
        return Token{"unknown", val};
    case '<':
        currentState = S_LESS;
        if (peek() == '>')
        {
            val += advance();
            currentState = S_NOTEQUAL;
            return Token{"neq", val};
        }
        else if (peek() == '=')
        {
            val += advance(); // Consume '='
            currentState = S_LESSEQUAL;
            return Token{"leq", val};
        }
        return Token{"lss", val};
    case '>':
        currentState = S_GREATER;
        if (peek() == '=')
        {
            val += advance(); // Consume '='
            currentState = S_GREATEREQUAL;
            return Token{"geq", val};
        }
        return Token{"gtr", val};
    default:
        currentState = S_ERROR;
        return Token{"unknown", val};
    }
};

Token Lexer::readPunctuation(char ch)
{
    string val(1, ch);

    switch (ch)
    {
    case ',':
        currentState = S_COMMA;
        return Token{"comma", val};
    case ';':
        currentState = S_SEMICOLON;
        return Token{"semicolon", val};
    case '.':
        currentState = S_PERIOD;
        return Token{"period", val};
    case ':':
        currentState = S_COLON;
        if (peek() == '=')
        {
            val += advance();
            currentState = S_BECOMES;
            return Token{"becomes", val};
        }
        return Token{"colon", val};

    case ')':
        currentState = S_RPAR;
        return Token{"rparent", val};
    case '[':
        currentState = S_LBRACK;
        return Token{"lbrack", val};
    case ']':
        currentState = S_RBRACK;
        return Token{"rbrack", val};
    default:
        currentState = S_ERROR;
        return Token{"unknown", val};
    }
};

string Lexer::classifyKeyword(string val)
{
    string lower = val;
    transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::map<string, string> keywords = {{"const", "constsy"}, {"case", "casesy"}, {"var", "varsy"}, {"function", "functionsy"}, {"for", "forsy"}, {"array", "arraysy"}, {"record", "recordsy"}, {"repeat", "repeatsy"}, {"if", "ifsy"}, {"while", "whilesy"}, {"end", "endsy"}, {"else", "elsesy"}, {"of", "ofsy"}, {"do", "dosy"}, {"downto", "downtosy"}, {"procedure", "proceduresy"}, {"program", "programsy"}, {"until", "untilsy"}, {"begin", "beginsy"}, {"type", "typesy"}, {"then", "thensy"}, {"to", "tosy"}, {"not", "notsy"}, {"and", "andsy"}, {"or", "orsy"}, {"div", "idiv"}, {"mod", "imod"}};
    // "array", "begin", "case", "const", "do", "downto", "else", "end", "for", "function", "if", "of", "procedure", "program", "record", "repeat", "then", "to", "type", "until", "var", "while"

    auto it = keywords.find(lower);
    if (it != keywords.end())
    {
        currentState = S_KEYWORD;
        return it->second;
    }

    currentState = S_IDENT;
    return "ident";
};

Token Lexer::readIdentOrKeyword(char ch)
{
    string val = "";
    val += ch;
    currentState = S_KEY_INPUT;
    advance();

    while (isalnum(peek()))
    {
        val += peek();
        advance();
    }

    string lower = val;
    transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    currentState = S_KEY_CLASSIFY;
    return Token{classifyKeyword(val), lower};
};

Token Lexer::readUnknown(char ch)
{
    string val(1, ch);
    currentState = S_UNKNOWN;

    while (peek() != '\0' && !isspace(peek()))
    {
        val += peek();
        advance();
    }

    return Token{"unknown", val};
}

vector<Token> Lexer::tokenize()
{
    vector<Token> tokens;

    while (peek() != '\0')
    {

        if (isspace(peek()))
        {
            advance();
            currentState = S0;
            continue;
        }

        // if ((currentState != S_INT && currentState != S_REAL) && peek() != '-')
        // {
        //     currentState = S0;
        // }

        char ch = peek();

        // if (isspace(ch))
        // {
        //     advance();
        //     continue;
        // }

        // read Number
        if (isdigit(ch))
        {
            tokens.push_back(readNumber());
        }
        // read Identifier(variable name) or Keyword
        else if (isalpha(ch))
        {
            tokens.push_back(readIdentOrKeyword(ch));
        }
        // read String
        else if (ch == '\'')
        {
            tokens.push_back(readString());
        }
        // read Comment
        else if (ch == '{')
        {
            tokens.push_back(readComment(ch));
        }
        else if (ch == '(')
        {
            advance();
            if (peek() == '*')
            {
                tokens.push_back(readComment(ch));
            }
            else
            {
                currentState = S_LPAR;
                tokens.push_back({"lparent", "("});
            }
        }
        // read operator
        else if (ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
                 ch == '=' || ch == '<' || ch == '>')
        {
            advance();
            tokens.push_back(readOperator(ch));
        }

        // Fix milestone 1: titik cuman valid kalau setelahnya spasi, /0, atau '.' lagi.
        else if (ch == '.')
        {
            char next = (static_cast<size_t>(pos + 1) < source.size()) ? source[pos + 1] : '\0';
            // kasus valid
            if (next == '.' || next == '\0' || isspace(next))
            {
                advance();
                tokens.push_back(readPunctuation(ch));
            }
            else
            {
                // kasus diikuti non-seperator
                advance();
                tokens.push_back(readUnknown(ch));
            }
        }

        // read punctuation
        else if (ch == ',' || ch == '.' || ch == ';' || ch == ':' ||
                 ch == ')' || ch == '[' || ch == ']')
        {
            advance();
            tokens.push_back(readPunctuation(ch));
        }

        // Karakter tidak dikenal
        else
        {
            tokens.push_back(Token({"", string(1, ch)}));
            advance();
        }
    }
    return tokens;
};