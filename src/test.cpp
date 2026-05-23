#include <iostream>
#include "semanticAnalyzer.h"

using namespace std;

// =============================================================
// Helper: cetak pemisah antar test case
// =============================================================
void printSeparator(const string &title)
{
    cout << "\n";
    cout << "=============================================================\n";
    cout << "  " << title << "\n";
    cout << "=============================================================\n";
}

// =============================================================
// TEST 1: Program sederhana dengan var dan const
//
// Simulasi:
//   program Hello;
//   var
//     a, b: integer;
//   const
//     MAX = 100;
//   begin
//   end.
//
// Yang diuji:
//   - visitProgram: nama program masuk tab
//   - visitVarDecl: a dan b masuk tab sebagai VARIABLE INTEGER
//   - visitConstDecl: MAX masuk tab sebagai CONSTANT INTEGER
//   - Tidak ada error
// =============================================================
void test1_basicVarConst()
{
    printSeparator("TEST 1: Var dan Const sederhana (tidak ada error)");

    SymbolTable st;
    auto prog = make_shared<ProgramNode>("Hello");

    prog->declarations.push_back(make_shared<VarDeclNode>("a", DataType::INTEGER));
    prog->declarations.push_back(make_shared<VarDeclNode>("b", DataType::INTEGER));
    prog->declarations.push_back(make_shared<ConstDeclNode>("MAX", DataType::INTEGER, "100"));
    prog->body = make_shared<CompoundStmtNode>();

    SemanticAnalyzer analyzer(st);
    analyzer.analyze(prog);

    prog->print(cout, "", true, true);
    cout << endl;
    st.printTables();
    printErrorSummary();
    resetErrorHandler();
}

// =============================================================
// TEST 2: Redeclaration variable
//
// Simulasi:
//   program Test2;
//   var
//     a: integer;
//     a: real;     <-- redeclaration, harus error
//   begin
//   end.
//
// Yang diuji:
//   - visitVarDecl mendeteksi redeclaration di scope yang sama
//   - Error count = 1
// =============================================================
void test2_redeclaration()
{
    printSeparator("TEST 2: Redeclaration variable (harus 1 error)");

    SymbolTable st;
    auto prog = make_shared<ProgramNode>("Test2");

    prog->declarations.push_back(make_shared<VarDeclNode>("a", DataType::INTEGER));
    prog->declarations.push_back(make_shared<VarDeclNode>("a", DataType::REAL)); // redeclaration
    prog->body = make_shared<CompoundStmtNode>();

    SemanticAnalyzer analyzer(st);
    analyzer.analyze(prog);

    prog->print(cout, "", true, true);
    cout << endl;
    st.printTables();
    printErrorSummary();
    resetErrorHandler();
}

// =============================================================
// TEST 3: Prosedur dengan parameter
//
// Simulasi:
//   program Test3;
//   procedure hitung(x: integer; y: real);
//   begin
//   end;
//   begin
//   end.
//
// Yang diuji:
//   - visitProcDecl: hitung masuk tab sebagai PROCEDURE
//   - x dan y masuk tab di level 1 (scope prosedur)
//   - pushScope dan popScope berjalan benar
//   - btab punya 2 block: global (0) dan prosedur (1)
// =============================================================
void test3_procedure()
{
    printSeparator("TEST 3: Prosedur dengan parameter");

    SymbolTable st;
    auto prog = make_shared<ProgramNode>("Test3");

    auto proc = make_shared<ProcDeclNode>("hitung");
    proc->params.push_back(make_shared<VarDeclNode>("x", DataType::INTEGER));
    proc->params.push_back(make_shared<VarDeclNode>("y", DataType::REAL));
    proc->body = make_shared<CompoundStmtNode>();

    prog->declarations.push_back(proc);
    prog->body = make_shared<CompoundStmtNode>();

    SemanticAnalyzer analyzer(st);
    analyzer.analyze(prog);

    prog->print(cout, "", true, true);
    cout << endl;
    st.printTables();
    printErrorSummary();
    resetErrorHandler();
}

// =============================================================
// TEST 4: Fungsi dengan return type
//
// Simulasi:
//   program Test4;
//   function tambah(a: integer; b: integer): integer;
//   begin
//   end;
//   begin
//   end.
//
// Yang diuji:
//   - visitFuncDecl: tambah masuk tab sebagai FUNCTION dengan type INTEGER
//   - Parameter a dan b masuk tab di scope fungsi
// =============================================================
void test4_function()
{
    printSeparator("TEST 4: Fungsi dengan return type");

    SymbolTable st;
    auto prog = make_shared<ProgramNode>("Test4");

    auto func = make_shared<FuncDeclNode>("tambah", DataType::INTEGER);
    func->params.push_back(make_shared<VarDeclNode>("a", DataType::INTEGER));
    func->params.push_back(make_shared<VarDeclNode>("b", DataType::INTEGER));
    func->body = make_shared<CompoundStmtNode>();

    prog->declarations.push_back(func);
    prog->body = make_shared<CompoundStmtNode>();

    SemanticAnalyzer analyzer(st);
    analyzer.analyze(prog);

    prog->print(cout, "", true, true);
    cout << endl;
    st.printTables();
    printErrorSummary();
    resetErrorHandler();
}

// =============================================================
// TEST 5: Nested prosedur (prosedur di dalam prosedur)
//
// Simulasi:
//   program Test5;
//   procedure outer(x: integer);
//     procedure inner(y: boolean);
//     begin
//     end;
//   begin
//   end;
//   begin
//   end.
//
// Yang diuji:
//   - Scope bersarang: outer di level 1, inner di level 2
//   - x dan y berada di scope yang berbeda
//   - Setelah selesai, level kembali ke 0
// =============================================================
void test5_nestedProcedure()
{
    printSeparator("TEST 5: Prosedur bersarang (nested)");

    SymbolTable st;
    auto prog = make_shared<ProgramNode>("Test5");

    // inner procedure — dideklarasikan di dalam scope outer
    auto inner = make_shared<ProcDeclNode>("inner");
    inner->params.push_back(make_shared<VarDeclNode>("y", DataType::BOOLEAN));
    inner->body = make_shared<CompoundStmtNode>();

    // outer procedure
    auto outer = make_shared<ProcDeclNode>("outer");
    outer->params.push_back(make_shared<VarDeclNode>("x", DataType::INTEGER));

    // inner dimasukkan ke body outer sebagai statement.
    // visitCompoundStmt akan meneruskannya ke visitStatement,
    // yang kemudian memanggil visitProcDecl(inner) di scope level 1.
    auto outerBody = make_shared<CompoundStmtNode>();
    outerBody->statements.push_back(inner);
    outer->body = outerBody;

    prog->declarations.push_back(outer);
    prog->body = make_shared<CompoundStmtNode>();

    SemanticAnalyzer analyzer(st);
    analyzer.analyze(prog);

    prog->print(cout, "", true, true);
    cout << endl;
    st.printTables();
    printErrorSummary();
    resetErrorHandler();
}

// =============================================================
// TEST 6: Type declaration
//
// Simulasi:
//   program Test6;
//   type
//     MyInt = integer;   (alias tipe sederhana)
//   var
//     n: integer;
//   begin
//   end.
//
// Yang diuji:
//   - visitTypeDecl: MyInt masuk tab sebagai TYPE
//   - resolveTypeName bisa menemukan tipe yang sudah dideklarasikan
// =============================================================
void test6_typeDecl()
{
    printSeparator("TEST 6: Type declaration");

    SymbolTable st;
    auto prog = make_shared<ProgramNode>("Test6");

    // type MyInt = integer (alias, baseType=INTEGER, ref=0)
    prog->declarations.push_back(make_shared<TypeDeclNode>("MyInt", DataType::INTEGER, 0));
    prog->declarations.push_back(make_shared<VarDeclNode>("n", DataType::INTEGER));
    prog->body = make_shared<CompoundStmtNode>();

    SemanticAnalyzer analyzer(st);
    analyzer.analyze(prog);

    prog->print(cout, "", true, true);
    cout << endl;
    st.printTables();
    printErrorSummary();
    resetErrorHandler();
}

// =============================================================
// TEST 7: Program lengkap — kombinasi semua deklarasi
//
// Simulasi:
//   program Lengkap;
//   const
//     PI = 3;
//   type
//     Score = integer;
//   var
//     x, y: integer;
//     flag: boolean;
//   function max(a: integer; b: integer): integer;
//   begin
//   end;
//   procedure cetak(val: integer);
//   begin
//   end;
//   begin
//   end.
// =============================================================
void test7_combined()
{
    printSeparator("TEST 7: Program lengkap (kombinasi semua deklarasi)");

    SymbolTable st;
    auto prog = make_shared<ProgramNode>("Lengkap");

    // const PI = 3
    prog->declarations.push_back(make_shared<ConstDeclNode>("PI", DataType::INTEGER, "3"));

    // type Score = integer
    prog->declarations.push_back(make_shared<TypeDeclNode>("Score", DataType::INTEGER, 0));

    // var x, y: integer
    prog->declarations.push_back(make_shared<VarDeclNode>("x", DataType::INTEGER));
    prog->declarations.push_back(make_shared<VarDeclNode>("y", DataType::INTEGER));

    // var flag: boolean
    prog->declarations.push_back(make_shared<VarDeclNode>("flag", DataType::BOOLEAN));

    // function max(a, b: integer): integer
    auto func = make_shared<FuncDeclNode>("max", DataType::INTEGER);
    func->params.push_back(make_shared<VarDeclNode>("a", DataType::INTEGER));
    func->params.push_back(make_shared<VarDeclNode>("b", DataType::INTEGER));
    func->body = make_shared<CompoundStmtNode>();
    prog->declarations.push_back(func);

    // procedure cetak(val: integer)
    auto proc = make_shared<ProcDeclNode>("cetak");
    proc->params.push_back(make_shared<VarDeclNode>("val", DataType::INTEGER));
    proc->body = make_shared<CompoundStmtNode>();
    prog->declarations.push_back(proc);

    prog->body = make_shared<CompoundStmtNode>();

    SemanticAnalyzer analyzer(st);
    analyzer.analyze(prog);

    prog->print(cout, "", true, true);
    cout << endl;
    st.printTables();
    printErrorSummary();
    resetErrorHandler();
}

// =============================================================
// Main
// =============================================================
int main()
{
    test1_basicVarConst();
    test2_redeclaration();
    test3_procedure();
    test4_function();
    test5_nestedProcedure();
    test6_typeDecl();
    test7_combined();

    return 0;
}