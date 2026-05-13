#pragma once
#include "symbolTable.hpp"
#include <string>

struct SourceLocation
{
    int line;
    int col;

    SourceLocation(int l = 0, int c = 0) : line(l), col(c) {}

    std::string toString() const;
};

enum class ErrorLevel
{
    WARNING,
    ERROR,
    FATAL
};

// Error generik ketika tidak ada fungsi spesifik yang cocok
void semanticError(const std::string &message,
                   const SourceLocation &loc = SourceLocation(),
                   ErrorLevel level = ErrorLevel::ERROR);

// Identifier dipakai tapi belum dideklarasikan
void undeclaredError(const std::string &identName,
                     const SourceLocation &loc = SourceLocation());

// Identifier dideklarasikan ulang di scope yang sama
void redeclarationError(const std::string &identName,
                        const SourceLocation &loc = SourceLocation());

// Type mismatch antara dua type
void typeMismatchError(DataType expected, DataType got,
                       const std::string &context = "",
                       const SourceLocation &loc = SourceLocation());

// Assignment-incompatible: T2 tidak bisa diassign ke T1
void assignIncompatibleError(DataType targetType, DataType valueType,
                             const std::string &varName = "",
                             const SourceLocation &loc = SourceLocation());

// Operator biner diaplikasikan pada tipe yang tidak valid
void invalidOperandError(const std::string &op,
                         DataType leftType, DataType rightType,
                         const SourceLocation &loc = SourceLocation());

// Operator unary diaplikasikan pada tipe yang tidak valid"
void invalidUnaryOperandError(const std::string &op,
                              DataType operandType,
                              const SourceLocation &loc = SourceLocation());

// Kondisi if/while/repeat bukan Boolean"
void nonBooleanConditionError(const std::string &stmtType,
                              DataType got,
                              const SourceLocation &loc = SourceLocation());

// Lower bound subrange > upper bound
void invalidSubrangeError(int low, int high,
                          const SourceLocation &loc = SourceLocation());

// Index type array tidak valid (misal Real)
void invalidIndexTypeError(DataType indexType,
                           const SourceLocation &loc = SourceLocation());

// Jumlah argumen prosedur/fungsi tidak sesuai
void wrongArgCountError(const std::string &procName,
                        int expected, int got,
                        const SourceLocation &loc = SourceLocation());

// Identifier dipakai sebagai variable tapi obj-nya bukan variable/constant
void wrongObjectKindError(const std::string &identName,
                          AllowedObj expected, AllowedObj got,
                          const SourceLocation &loc = SourceLocation());

// Subrange tidak boleh bertipe Real
void realSubrangeError(const SourceLocation &loc = SourceLocation());

int getErrorCount();
int getWarningCount();
bool hasFatalError();

// Reset untuk unit testing
void resetErrorHandler();

// Print ringkasan di akhir kompilasi
void printErrorSummary();