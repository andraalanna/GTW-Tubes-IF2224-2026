#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "Parser.h"

using namespace std;

void printTree(shared_ptr<ParseNode> node, ostream &out, string prefix = "", bool isLast = true, bool isRoot = true)
{
    if (!node)
        return;

    if (isRoot)
    {
        out << node->type;
        if (!node->value.empty())
        {
            out << "(" << node->value << ")";
        }
        out << endl;
    }
    else
    {
        out << prefix;
        out << (isLast ? "└── " : "├── ");

        if (tokenHasValue(node->type))
        {
            out << node->type << "(" << node->value << ")" << endl;
        }
        else
        {
            out << node->type << endl;
        }
    }

    for (size_t i = 0; i < node->children.size(); i++)
    {
        printTree(
            node->children[i],
            out,
            prefix + (isRoot ? "" : (isLast ? "    " : "│   ")),
            i == node->children.size() - 1,
            false);
    }
}

int main(int argc, char *argv[])
{
    // Contoh input: ./arion input.txt output.txt
    if (argc < 3)
    {
        cerr << "Usage: ./arion input.txt output.txt" << endl;
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

    try
    {
        Parser parser(tokens);
        shared_ptr<ParseNode> root = parser.parse();

        ofstream outFile(argv[2]);
        if (!outFile)
        {
            cerr << "ERROR: tidak bisa membuat file: " << argv[2] << endl;
            return 1;
        }

        // Print ke terminal
        printTree(root, cout);

        // Simpan ke file output
        printTree(root, outFile);

        outFile.close();
        cout << "Selesai! Parse tree tersimpan di " << argv[2] << endl;
    }
    catch (const exception &e)
    {
        cerr << e.what() << endl;

        ofstream outFile(argv[2]);
        if (outFile)
        {
            outFile << e.what() << endl;
            outFile.close();
        }

        return 1;
    }

    return 0;
}