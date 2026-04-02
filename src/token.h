#ifndef TOKEN_H
#define TOKEN_H

#include <string>
using namespace std;

/**
 * Tipe Token teridiri dari:
 *  string Type : nama jenis token (intcon, ident, plus, beginsy, etc)
 *  string Vakue : karakter asli dari source code ("48", "begin:, "+", etc)
 */

struct Token {
    string type;
    string value;
};

// Mengecek apakah token perlu ditampilkan beserta value-nya
// Token seperti intcon, ident, string perlu ditampilkan nilainya
// Token seperti plus, semicolon, beginsy tidak perlu
inline bool tokenHasValue(const string& type) {
    return type == "intcon"  ||
           type == "realcon" ||
           type == "charcon" ||
           type == "string"  ||
           type == "ident"   ||
           type == "comment" ||
           type == "unknown";
}

#endif