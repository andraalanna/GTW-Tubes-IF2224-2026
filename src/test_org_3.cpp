#include "TypeSystem.h"
#include "ErrorHandler.h"
#include <cassert>
#include <iostream>

// Helper print
void section(const string &name)
{
    cout << "\n=== " << name << " ===\n";
}

// TEST 1 — isCompatible: semua kombinasi tipe
void test1_isCompatible()
{
    section("TEST 1: isCompatible");

    // Same type → compatible
    assert(isCompatible(DataType::INTEGER, DataType::INTEGER));
    assert(isCompatible(DataType::REAL, DataType::REAL));
    assert(isCompatible(DataType::CHAR, DataType::CHAR));
    assert(isCompatible(DataType::BOOLEAN, DataType::BOOLEAN));
    assert(isCompatible(DataType::STRING, DataType::STRING));

    // Subrange ↔ Integer/Char/Boolean → compatible
    assert(isCompatible(DataType::SUBRANGE, DataType::INTEGER));
    assert(isCompatible(DataType::INTEGER, DataType::SUBRANGE));
    assert(isCompatible(DataType::SUBRANGE, DataType::CHAR));
    assert(isCompatible(DataType::SUBRANGE, DataType::SUBRANGE));

    // Subrange ↔ Real → TIDAK compatible
    assert(!isCompatible(DataType::SUBRANGE, DataType::REAL));
    assert(!isCompatible(DataType::REAL, DataType::SUBRANGE));

    // Beda tipe dasar → TIDAK compatible
    assert(!isCompatible(DataType::INTEGER, DataType::BOOLEAN));
    assert(!isCompatible(DataType::CHAR, DataType::REAL));
    assert(!isCompatible(DataType::STRING, DataType::INTEGER));

    // UNKNOWN → selalu false
    assert(!isCompatible(DataType::UNKNOWN, DataType::INTEGER));
    assert(!isCompatible(DataType::INTEGER, DataType::UNKNOWN));

    // RECORD: ref sama → compatible, ref beda → TIDAK
    assert(isCompatible(DataType::RECORD, DataType::RECORD, 1, 1));
    assert(!isCompatible(DataType::RECORD, DataType::RECORD, 1, 2)); // anonymous incompatible
    assert(!isCompatible(DataType::RECORD, DataType::RECORD, 0, 0)); // ref=0 → incompatible

    // ARRAY: ref sama → compatible, beda → TIDAK
    assert(isCompatible(DataType::ARRAY, DataType::ARRAY, 3, 3));
    assert(!isCompatible(DataType::ARRAY, DataType::ARRAY, 3, 4));

    cout << "PASS\n";
}

// TEST 2 — isAssignCompatible: widening + rule
void test2_isAssignCompatible()
{
    section("TEST 2: isAssignCompatible");

    // Same type → compatible
    assert(isAssignCompatible(DataType::INTEGER, DataType::INTEGER));
    assert(isAssignCompatible(DataType::BOOLEAN, DataType::BOOLEAN));
    assert(isAssignCompatible(DataType::CHAR, DataType::CHAR));
    assert(isAssignCompatible(DataType::STRING, DataType::STRING));

    // Widening: Real := Integer → OK
    assert(isAssignCompatible(DataType::REAL, DataType::INTEGER));

    // Narrowing: Integer := Real → TIDAK OK
    assert(!isAssignCompatible(DataType::INTEGER, DataType::REAL));

    // Boolean := Integer → TIDAK OK
    assert(!isAssignCompatible(DataType::BOOLEAN, DataType::INTEGER));

    // Subrange assign ke Integer → OK (subrange compatible dengan Integer)
    assert(isAssignCompatible(DataType::INTEGER, DataType::SUBRANGE));

    // RECORD: sama ref → OK, beda ref → TIDAK (anonymous incompatible)
    assert(isAssignCompatible(DataType::RECORD, DataType::RECORD, 5, 5));
    assert(!isAssignCompatible(DataType::RECORD, DataType::RECORD, 5, 6));

    // UNKNOWN → selalu false
    assert(!isAssignCompatible(DataType::UNKNOWN, DataType::INTEGER));
    assert(!isAssignCompatible(DataType::INTEGER, DataType::UNKNOWN));

    cout << "PASS\n";
}

// TEST 3 — inferBinOpType: semua operator
void test3_inferBinOpType()
{
    section("TEST 3: inferBinOpType");

    // Aritmatika integer
    assert(inferBinOpType("plus", DataType::INTEGER, DataType::INTEGER) == DataType::INTEGER);
    assert(inferBinOpType("minus", DataType::INTEGER, DataType::INTEGER) == DataType::INTEGER);
    assert(inferBinOpType("times", DataType::INTEGER, DataType::INTEGER) == DataType::INTEGER);

    // Aritmatika mixed → Real
    assert(inferBinOpType("plus", DataType::INTEGER, DataType::REAL) == DataType::REAL);
    assert(inferBinOpType("minus", DataType::REAL, DataType::INTEGER) == DataType::REAL);
    assert(inferBinOpType("times", DataType::REAL, DataType::REAL) == DataType::REAL);

    // rdiv → selalu Real
    assert(inferBinOpType("rdiv", DataType::INTEGER, DataType::INTEGER) == DataType::REAL);
    assert(inferBinOpType("rdiv", DataType::REAL, DataType::REAL) == DataType::REAL);
    assert(inferBinOpType("rdiv", DataType::REAL, DataType::INTEGER) == DataType::REAL);

    // idiv dan imod → hanya Integer
    assert(inferBinOpType("idiv", DataType::INTEGER, DataType::INTEGER) == DataType::INTEGER);
    assert(inferBinOpType("imod", DataType::INTEGER, DataType::INTEGER) == DataType::INTEGER);
    assert(inferBinOpType("idiv", DataType::REAL, DataType::INTEGER) == DataType::UNKNOWN); // Real tidak boleh idiv
    assert(inferBinOpType("imod", DataType::INTEGER, DataType::REAL) == DataType::UNKNOWN);

    // Logika → Boolean
    assert(inferBinOpType("andsy", DataType::BOOLEAN, DataType::BOOLEAN) == DataType::BOOLEAN);
    assert(inferBinOpType("orsy", DataType::BOOLEAN, DataType::BOOLEAN) == DataType::BOOLEAN);
    assert(inferBinOpType("andsy", DataType::INTEGER, DataType::BOOLEAN) == DataType::UNKNOWN); // INTEGER bukan Boolean
    assert(inferBinOpType("orsy", DataType::BOOLEAN, DataType::INTEGER) == DataType::UNKNOWN);

    // Relasional → Boolean
    assert(inferBinOpType("eql", DataType::INTEGER, DataType::INTEGER) == DataType::BOOLEAN);
    assert(inferBinOpType("neq", DataType::CHAR, DataType::CHAR) == DataType::BOOLEAN);
    assert(inferBinOpType("lss", DataType::REAL, DataType::INTEGER) == DataType::BOOLEAN); // mixed ok untuk relasional
    assert(inferBinOpType("geq", DataType::STRING, DataType::STRING) == DataType::BOOLEAN);
    assert(inferBinOpType("gtr", DataType::ARRAY, DataType::ARRAY) == DataType::UNKNOWN); // ARRAY tidak bisa relasional

    // UNKNOWN propagation
    assert(inferBinOpType("plus", DataType::UNKNOWN, DataType::INTEGER) == DataType::UNKNOWN);
    assert(inferBinOpType("eql", DataType::INTEGER, DataType::UNKNOWN) == DataType::UNKNOWN);

    cout << "PASS\n";
}

// TEST 4 — inferUnaryOpType: operator yang unary sifatnya
void test4_inferUnaryOpType()
{
    section("TEST 4: inferUnaryOpType");

    // not → hanya Boolean
    assert(inferUnaryOpType("notsy", DataType::BOOLEAN) == DataType::BOOLEAN);
    assert(inferUnaryOpType("notsy", DataType::INTEGER) == DataType::UNKNOWN);
    assert(inferUnaryOpType("notsy", DataType::REAL) == DataType::UNKNOWN);

    // unary plus/minus → Integer atau Real
    assert(inferUnaryOpType("plus", DataType::INTEGER) == DataType::INTEGER);
    assert(inferUnaryOpType("minus", DataType::INTEGER) == DataType::INTEGER);
    assert(inferUnaryOpType("plus", DataType::REAL) == DataType::REAL);
    assert(inferUnaryOpType("minus", DataType::REAL) == DataType::REAL);

    // unary plus/minus pada Boolean/Char → UNKNOWN
    assert(inferUnaryOpType("plus", DataType::BOOLEAN) == DataType::UNKNOWN);
    assert(inferUnaryOpType("minus", DataType::CHAR) == DataType::UNKNOWN);

    // UNKNOWN propagation
    assert(inferUnaryOpType("notsy", DataType::UNKNOWN) == DataType::UNKNOWN);
    assert(inferUnaryOpType("minus", DataType::UNKNOWN) == DataType::UNKNOWN);

    cout << "PASS\n";
}

// TEST 5 — ErrorHandler: semua fungsi error
void test5_errorHandler()
{
    section("TEST 5: ErrorHandler");

    resetErrorHandler();
    assert(getErrorCount() == 0);
    assert(getWarningCount() == 0);
    assert(!hasFatalError());

    // undeclaredError
    undeclaredError("x", {5, 3});
    assert(getErrorCount() == 1);

    // redeclarationError
    redeclarationError("y", {10, 1});
    assert(getErrorCount() == 2);

    // typeMismatchError
    typeMismatchError(DataType::INTEGER, DataType::REAL, "assignment", {15, 7});
    assert(getErrorCount() == 3);

    // assignIncompatibleError
    assignIncompatibleError(DataType::INTEGER, DataType::REAL, "index", {20, 5});
    assert(getErrorCount() == 4);

    // invalidOperandError
    invalidOperandError("plus", DataType::BOOLEAN, DataType::INTEGER, {25, 10});
    assert(getErrorCount() == 5);

    // invalidUnaryOperandError
    invalidUnaryOperandError("notsy", DataType::INTEGER, {30, 2});
    assert(getErrorCount() == 6);

    // nonBooleanConditionError
    nonBooleanConditionError("if", DataType::INTEGER, {35, 4});
    assert(getErrorCount() == 7);

    // invalidSubrangeError: low > high
    invalidSubrangeError(10, 1, {40, 6});
    assert(getErrorCount() == 8);

    // invalidIndexTypeError: Real sebagai index
    invalidIndexTypeError(DataType::REAL, {45, 3});
    assert(getErrorCount() == 9);

    // wrongArgCountError
    wrongArgCountError("writeln", 2, 3, {50, 1});
    assert(getErrorCount() == 10);

    // wrongObjectKindError
    wrongObjectKindError("myProc", AllowedObj::VARIABLE, AllowedObj::PROCEDURE, {55, 1});
    assert(getErrorCount() == 11);

    // realSubrangeError
    realSubrangeError({60, 1});
    assert(getErrorCount() == 12);

    // semanticError dengan WARNING tidak naikkan errorCount
    semanticError("test warning", {65, 1}, ErrorLevel::WARNING);
    assert(getErrorCount() == 12); // tetap 12
    assert(getWarningCount() == 1);

    // FATAL naikkan errorCount dan set hasFatalError
    semanticError("fatal test", {70, 1}, ErrorLevel::FATAL);
    assert(getErrorCount() == 13);
    assert(hasFatalError());

    // printErrorSummary untuk visual check
    printErrorSummary();

    // Reset bersih
    resetErrorHandler();
    assert(getErrorCount() == 0);
    assert(getWarningCount() == 0);
    assert(!hasFatalError());

    cout << "PASS\n";
}

// MAIN
int main()
{
    test1_isCompatible();
    test2_isAssignCompatible();
    test3_inferBinOpType();
    test4_inferUnaryOpType();
    test5_errorHandler();

    cout << "\n✅ Semua test PASS.\n";
    return 0;
}