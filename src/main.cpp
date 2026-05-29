#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "Parser.h"
#include "symbolTable.hpp"
#include "ASTNode.h"
#include "semanticAnalyzer.h"
#include "ErrorHandler.h"
#include "ASTBuilder.h"
#include "ICG.h"
#include "Interpreter.hpp"
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

void testSymbolTable(shared_ptr<ParseNode> root)
{
    SymbolTable symTable;

    cout << "\n[1] init selesai, predefined masuk indeks 1-9, user start dari " << PredefinedIdx::USER_START << endl;
    symTable.printTables();

    cout << "\n[2] enterTab manual..." << endl;
    int idxHello = symTable.enterTab("Hello", AllowedObj::PROGRAM, DataType::VOID, 0, 1, 0, 0);
    cout << "    Hello -> tab[" << idxHello << "]" << endl;

    int idxA = symTable.enterTab("a", AllowedObj::VARIABLE, DataType::INTEGER, 0, 1, symTable.currentLevel, 0);
    cout << "    a -> tab[" << idxA << "]" << endl;

    int idxB = symTable.enterTab("b", AllowedObj::VARIABLE, DataType::INTEGER, 0, 1, symTable.currentLevel, 1);
    cout << "    b -> tab[" << idxB << "]" << endl;

    cout << "\n[3] lookup..." << endl;
    int found = symTable.lookup("a");
    cout << "    lookup(a) -> " << found << (found != -1 ? " ketemu" : " tidak ketemu") << endl;

    found = symTable.lookup("Integer");
    cout << "    lookup(Integer) -> " << found << (found != -1 ? " ketemu" : " tidak ketemu") << endl;

    found = symTable.lookup("writeln");
    cout << "    lookup(writeln) -> " << found << (found != -1 ? " ketemu" : " tidak ketemu") << endl;

    found = symTable.lookup("x");
    cout << "    lookup(x) -> " << found << (found != -1 ? " ketemu" : " tidak ketemu") << endl;

    cout << "\n[4] lookupCurrentScope..." << endl;
    found = symTable.lookupCurrentScope("a");
    cout << "    lookupCurrentScope(a) -> " << found << (found != -1 ? " redeclaration" : " aman") << endl;

    found = symTable.lookupCurrentScope("c");
    cout << "    lookupCurrentScope(c) -> " << found << (found != -1 ? " ada" : " aman") << endl;

    cout << "\n[5] pushScope & popScope..." << endl;
    cout << "    level sebelum: " << symTable.currentLevel << endl;
    int newBlock = symTable.pushScope();
    cout << "    pushScope -> btab[" << newBlock << "] level: " << symTable.currentLevel << endl;

    int idxC = symTable.enterTab("c", AllowedObj::VARIABLE, DataType::INTEGER, 0, 1, symTable.currentLevel, 0);
    cout << "    c -> tab[" << idxC << "]" << endl;

    found = symTable.lookup("a");
    cout << "    lookup(a) dari dalam -> " << found << (found != -1 ? " ketemu" : " tidak ketemu") << endl;

    symTable.popScope();
    cout << "    popScope -> level: " << symTable.currentLevel << endl;

    found = symTable.lookup("c");
    cout << "    lookup(c) setelah pop -> " << found << (found != -1 ? " ketemu" : " tidak ketemu") << endl;

    cout << "\n[6] tabel akhir:" << endl;
    symTable.printTables();
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
        const vector<string> &errors = parser.getErrors();

        if (!errors.empty())
        {
            ofstream outFile(argv[2]);
            for (const string &err : errors)
            {
                cerr << err << endl;
                if (outFile)
                    outFile << err << endl;
            }
            if (outFile)
                outFile.close();
            return 1;
        }

        ofstream outFile(argv[2]);
        if (!outFile)
        {
            cerr << "ERROR: tidak bisa membuat file: " << argv[2] << endl;
            return 1;
        }

        // Print ke terminal
        printTree(root, cout);

        // Simpan ke file output
        // printTree(root, outFile);

        // Build AST and run Semantic Analyzer
        SymbolTable symTable;
        ASTBuilder builder(symTable);
        auto astRoot = builder.build(root);

        SemanticAnalyzer analyzer(symTable);
        analyzer.analyze(astRoot);

        if (getErrorCount() == 0 && !hasFatalError())
        {
            ICG icg(symTable);
            auto instrs = icg.generate(astRoot);

            cout << "\n=== Intermediate Code ===" << endl;
            outFile << "\n=== Intermediate Code ===" << endl;
            icg.printInstructions(cout);
            icg.printInstructions(outFile);

            cout << "\n=== Output Program ===\n";
            outFile << "\n=== Output Program ===\n";

            struct MultiBuf : public std::streambuf
            {
                MultiBuf(std::streambuf* b1, std::streambuf* b2) : buf1(b1), buf2(b2) {}
                virtual int overflow(int c) override
                {
                    if (c == EOF) return EOF;
                    int r1 = buf1->sputc(c);
                    int r2 = buf2->sputc(c);
                    return (r1 == EOF || r2 == EOF) ? EOF : c;
                }
                virtual int sync() override
                {
                    int r1 = buf1->pubsync();
                    int r2 = buf2->pubsync();
                    return (r1 == -1 || r2 == -1) ? -1 : 0;
                }
                std::streambuf* buf1;
                std::streambuf* buf2;
            };

            MultiBuf multi(cout.rdbuf(), outFile.rdbuf());
            ostream multiOut(&multi);

            Interpreter interpreter(multiOut);
            interpreter.stringTable = icg.stringTable;
            interpreter.realPool = icg.realPool;

            try
            {
                interpreter.run(instrs);
            }
            catch (const RuntimeError &e)
            {
                multiOut << e.what() << endl;
            }
        }

        cout << "\n=== Symbol Table ===" << endl;
        symTable.printTables();

        // if (astRoot) astRoot->print(cout);
        // Print summary ke outFile
        cout << "\n=== Semantic Analysis ===" << endl;
        cout << "  Errors  : " << getErrorCount() << endl;
        cout << "  Warnings: " << getWarningCount() << endl;
        if (getErrorCount() == 0 && !hasFatalError())
            cout << "  Status  : OK" << endl;
        else
            cout << "  Status  : FAILED" << endl;

        cout << "\n=== Decorated AST ===" << endl;
        if (astRoot)
            astRoot->print(cout);

        printErrorSummary();

        if (astRoot)
        {
            outFile << "\n=== Decorated AST ===\n";
            astRoot->print(outFile);
        }
        outFile.close();
    }

    catch (const SyntaxError &e)
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

    catch (const exception &e)
    {
        cerr << "Unrecoverable error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
