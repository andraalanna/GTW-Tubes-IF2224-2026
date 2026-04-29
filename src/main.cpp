#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"

using namespace std;

int main(int argc, char *argv[])
{
    // Contoh input: ./lexer input.txt output.txt
    if (argc < 3)
    {
        cerr << "Usage: ./lexer input.txt output.txt" << endl;
        return 1;
    }

    ifstream inputFile(argv[1]);
    if (!inputFile)
    {
        cerr << "ERROR: file tidak ditemukan: " << argv[1] << endl;
        return 1;
    }

    stringstream ss;
    ss << inputFile.rdbuf();
    string source = ss.str();

    inputFile.close();

    Lexer lexer(source);

    vector<Token> tokens = lexer.tokenize();

    ofstream outFile(argv[2]);
    if (!outFile)
    {
        cerr << "ERROR: tidak bisa membuat file: " << argv[2] << endl;
        return 1;
    }

    for (Token t : tokens)
    {
        if (tokenHasValue(t.type))
            outFile << t.type << "(" << t.value << ")" << endl;
        else
            outFile << t.type << endl;
    }

    outFile.close();
    cout << "Selesai! Output tersimpan di " << argv[2] << endl;

    return 0;
}