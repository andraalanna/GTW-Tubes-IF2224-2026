#include "lexer.h"
#include <cctype>
#include <iostream>

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

    currentState = S_INT;

    while (isdigit(peek()))
    {
        val += advance();
    }

    
    if (peek() == '.')
    {
        currentState = S_REAL_DOT;
        val += advance();

        if (!isdigit(peek())){
            currentState = S_ERROR;
            return Token{"error", val};
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
            return Token{"error", "unterminated comment"};
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
        return Token{"error", "unterminated comment"};
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
            return Token{"error", "unterminated string"};
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
                    return Token{"string", ""};
                }
                if (val.size() == 1)
                {
                    currentState = S_CHAR;
                    return Token{"charcon", val};
                }
                currentState = S_QUOTE;
                return Token{"string", val};
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
        return Token{"error", val};
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
        return Token{"error", val};
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
        return Token{"error", val};
    }
};

string Lexer::classifyKeyword(string val)
{
    return "hai";
};

Token Lexer::readIdentOrKeyword()
{
    return Token{"string", ""};
};

vector<Token> Lexer::tokenize()
{
    vector<Token> tokens;

    while (peek() != '\0')
    {
        currentState = S0;
        char ch = peek();

        // skip whitespace
        if (isspace(ch))
        {
            advance();
            continue;
        }

        // read Number
        if (isdigit(ch))
        {
            tokens.push_back(readNumber());
        }

        // read Identifier(variable name) or Keyword(beginsy, termausuk -> MOD, AND, div, termasuk semua yang pakai string)
        else if (isalpha(ch))
        {
            tokens.push_back(readIdentOrKeyword());
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

        else if (ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
                 ch == '=' || ch == '<' || ch == '>')
        {
            advance();
            tokens.push_back(readOperator(ch));
        }

        else if (ch == ',' || ch == '.' || ch == ';' || ch == ':' ||
                 ch == ')' || ch == '[' || ch == ']')
        {
            advance();
            tokens.push_back(readPunctuation(ch));
        }

        // Karakter tidak dikenal
        else
        {
            tokens.push_back(Token{"Token", string(1, ch)});
            advance();
        }
    }
    return tokens;
};