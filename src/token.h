#ifndef TOKEN_H
#define TOKEN_H

#include <string>
using namespace std;

struct Token
{
    string type;
    string value;
    int line;
    int col;
};

inline bool tokenHasValue(const string &type)
{
    return type == "intcon" ||
           type == "realcon" ||
           type == "charcon" ||
           type == "string" ||
           type == "ident" ||
           type == "comment" ||
           type == "error";
}

#endif