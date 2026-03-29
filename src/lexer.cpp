#include "lexer.h"
#include <cctype>
#include <iostream>

using namespace std;

Lexer::Lexer(string src){
    source = src;
    pos = 0;
};

char Lexer::peek(){
    if (pos >= source.size()) return '\0';
    return source[pos];
};

char Lexer::advance(){
    if (pos >= source.size()) return '\0';
    return source[pos++];
};


Token Lexer::readNumber(){
    string val = "";

    while (isdigit(peek())){
        val += advance();
    }

    if (peek()== '.'){
        val += advance();
        while(isdigit(peek())){
            val += advance();
        }
        return Token({"realcon", val});
    }

    return Token({"intcon", val});
}

Token Lexer::readComment(char ch){};
Token Lexer::readString(){};

Token Lexer::readOperator(char ch){
    string val(1, ch);

    switch (ch) {
        case '+':
            return Token{"plus", val};
        case '-':
            return Token{"minus", val};
        case '*':
            return Token{"times", val};
        case '/':
            return Token{"rdiv", val};
        case '=':
            if (peek() == '=') {
                val += advance(); // Consume '=' yang kedua
                return Token{"eql", val};
            }
            return Token{"error", val}; 
        case '<':
            if (peek() == '>') {
                val += advance(); 
                return Token{"neq", val}; 
            } else if (peek() == '=') {
                val += advance(); // Consume '='
                return Token{"leq", val};
            }
            return Token{"lss", val};
        case '>':
            if (peek() == '=') {
                val += advance(); // Consume '='
                return Token{"geq", val};
            }
            return Token{"gtr", val};
        default:
            return Token{"error", val};
    }
};
Token Lexer::readPunctuation(char ch){};

string Lexer::classifyKeyword(string val){};
Token Lexer::readIdentOrKeyword(){};

vector<Token> Lexer::tokenize(){
    vector<Token> tokens;
    
    while (peek() != '\0'){
        char ch = peek();

        //skip whitespace
        if (isspace(ch)){
            advance();
            continue;
        }

        //read Number
        if (isdigit(ch)){
            tokens.push_back(readNumber());
        }

        //read Identifier(variable name) or Keyword(beginsy, termausuk -> MOD, AND, div, termasuk semua yang pakai string)
        else if (isalpha(ch)){
            tokens.push_back(readIdentOrKeyword());
        }

        //read String
        else if(ch == '\''){
            tokens.push_back(readString());
        }

        //read Comment
        else if(ch == '{'){
            tokens.push_back(readComment(ch));
        }
        else if(ch == '('){
            advance();
            if (ch == '*'){
                tokens.push_back(readComment(ch));
            }
            else {
                tokens.push_back({"lparent", "(" });
            }
        }

        else if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || 
            ch == '=' || ch == '<' || ch == '>' ){
                advance();
                tokens.push_back(readOperator(ch));
            }
        
        else if (ch == ',' ||  ch == '.' || ch == ';' || ch == ':' ||
            ch == ')' ||  ch == '[' ||  ch == ']'){
                advance();
                tokens.push_back(readPunctuation(ch));
            }

        //Karakter tidak dikenal
        else{
            tokens.push_back(Token{"Token", string(1, ch)});
            advance();
        }
        
        return tokens;

    }
};
