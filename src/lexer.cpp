#include "lexer.h"
#include <cctype>
#include <iostream>

using namespace std;

Lexer::Lexer(string src){};

char Lexer::peek(){};

char Lexer::advance(){};

string Lexer::classifyKeyword(string val){};

Token Lexer::readNumber(){};

Token Lexer::readString(){};
Token Lexer::readOperator(char ch){};
Token Lexer::readPunctuation(char ch){};
Token Lexer::readIdentOrKeyword(){};
vector<Token> Lexer::tokenize(){};
