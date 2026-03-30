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
    if (pos >= source.size())
        return '\0';
    return source[pos];
};

char Lexer::advance()
{
    if (pos >= source.size())
        return '\0';
    return source[pos++];
};

Token Lexer::readNumber()
{
    string val = "";

    while (isdigit(peek()))
    {
        val += advance();
    }

    if (peek() == '.')
    {
        val += advance();
        while (isdigit(peek()))
        {
            val += advance();
        }
        return Token({"realcon", val});
    }

    return Token({"intcon", val});
}

Token Lexer::readComment(char ch) {};
Token Lexer::readString() {};

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
    default:
        currentState = S_ERROR;
        return Token{"error", val};
    }
};

string Lexer::classifyKeyword(string val) {};

Token Lexer::readIdentOrKeyword() {};

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
            if (source[pos + 1] == '*')
            {              // Cek karakter setelah '('
                advance(); // Maju untuk '('
                advance(); // Maju untuk '*'
                tokens.push_back(readComment('*'));
            }
            else
            {
                advance(); // Cukup maju untuk '('
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