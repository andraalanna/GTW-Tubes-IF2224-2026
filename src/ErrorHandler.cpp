// src/error_handler.cpp
// Implementasi Error Handler - Milestone 3, Orang 3

#include "ErrorHandler.h"
#include "TypeSystem.h"
#include <iostream>
#include <sstream>

static int s_errorCount = 0;
static int s_warningCount = 0;
static bool s_fatalFlag = false;

std::string SourceLocation::toString() const
{
    if (line == 0 && col == 0)
        return "";
    std::ostringstream oss;
    oss << "line " << line << ", col " << col;
    return oss.str();
}

static void emit(ErrorLevel level, const std::string &message,
                 const SourceLocation &loc)
{
    std::string prefix;
    switch (level)
    {
    case ErrorLevel::WARNING:
        prefix = "[WARNING]";
        s_warningCount++;
        break;
    case ErrorLevel::ERROR:
        prefix = "[ERROR]";
        s_errorCount++;
        break;
    case ErrorLevel::FATAL:
        prefix = "[FATAL]";
        s_errorCount++;
        s_fatalFlag = true;
        break;
    }

    std::string locStr = loc.toString();
    std::cerr << prefix;
    if (!locStr.empty())
        std::cerr << " (" << locStr << ")";
    std::cerr << " " << message << std::endl;
}

void semanticError(const std::string &message,
                   const SourceLocation &loc,
                   ErrorLevel level)
{
    emit(level, message, loc);
}

void undeclaredError(const std::string &identName,
                     const SourceLocation &loc)
{
    emit(ErrorLevel::ERROR,
         "Undeclared identifier: '" + identName + "'",
         loc);
}

void redeclarationError(const std::string &identName,
                        const SourceLocation &loc)
{
    emit(ErrorLevel::ERROR,
         "Redeclaration of '" + identName + "' in the same scope",
         loc);
}

void typeMismatchError(DataType expected, DataType got,
                       const std::string &context,
                       const SourceLocation &loc)
{
    std::ostringstream oss;
    oss << "Type mismatch";
    if (!context.empty())
        oss << " in " << context;
    oss << ": expected '" << dataTypeToString(expected)
        << "', got '" << dataTypeToString(got) << "'";
    emit(ErrorLevel::ERROR, oss.str(), loc);
}

void assignIncompatibleError(DataType targetType, DataType valueType,
                             const std::string &varName,
                             const SourceLocation &loc)
{
    std::ostringstream oss;
    oss << "Assignment incompatible: cannot assign '"
        << dataTypeToString(valueType) << "' to '"
        << dataTypeToString(targetType) << "'";
    if (!varName.empty())
        oss << " (variable '" << varName << "')";
    emit(ErrorLevel::ERROR, oss.str(), loc);
}

void invalidOperandError(const std::string &op,
                         DataType leftType, DataType rightType,
                         const SourceLocation &loc)
{
    std::ostringstream oss;
    oss << "Invalid operand types for '" << op << "': '"
        << dataTypeToString(leftType) << "' and '"
        << dataTypeToString(rightType) << "'";
    emit(ErrorLevel::ERROR, oss.str(), loc);
}

void invalidUnaryOperandError(const std::string &op,
                              DataType operandType,
                              const SourceLocation &loc)
{
    std::ostringstream oss;
    oss << "Operator '" << op << "' cannot be applied to '"
        << dataTypeToString(operandType) << "'";
    emit(ErrorLevel::ERROR, oss.str(), loc);
}

void nonBooleanConditionError(const std::string &stmtType,
                              DataType got,
                              const SourceLocation &loc)
{
    std::ostringstream oss;
    oss << "Condition of '" << stmtType
        << "' must be Boolean, got '" << dataTypeToString(got) << "'";
    emit(ErrorLevel::ERROR, oss.str(), loc);
}

void invalidSubrangeError(int low, int high,
                          const SourceLocation &loc)
{
    std::ostringstream oss;
    oss << "Invalid subrange: lower bound " << low
        << " > upper bound " << high;
    emit(ErrorLevel::ERROR, oss.str(), loc);
}

void invalidIndexTypeError(DataType indexType,
                           const SourceLocation &loc)
{
    std::ostringstream oss;
    oss << "Invalid array index type: '"
        << dataTypeToString(indexType)
        << "' is not allowed as index type (Real is forbidden)";
    emit(ErrorLevel::ERROR, oss.str(), loc);
}

void wrongArgCountError(const std::string &procName,
                        int expected, int got,
                        const SourceLocation &loc)
{
    std::ostringstream oss;
    oss << "Wrong number of arguments for '" << procName
        << "': expected " << expected << ", got " << got;
    emit(ErrorLevel::ERROR, oss.str(), loc);
}

void wrongObjectKindError(const std::string &identName,
                          AllowedObj expected, AllowedObj got,
                          const SourceLocation &loc)
{
    std::ostringstream oss;
    oss << "'" << identName << "' is a "
        << SymbolTable::AllowedObjToString(got)
        << ", expected a "
        << SymbolTable::AllowedObjToString(expected);
    emit(ErrorLevel::ERROR, oss.str(), loc);
}

void realSubrangeError(const SourceLocation &loc)
{
    emit(ErrorLevel::ERROR,
         "Subrange type cannot be Real",
         loc);
}

int getErrorCount() { return s_errorCount; }
int getWarningCount() { return s_warningCount; }
bool hasFatalError() { return s_fatalFlag; }

void resetErrorHandler()
{
    s_errorCount = 0;
    s_warningCount = 0;
    s_fatalFlag = false;
}

void printErrorSummary()
{
    std::cerr << "\n=== Semantic Analysis ===" << std::endl;
    std::cerr << "  Errors  : " << s_errorCount << std::endl;
    std::cerr << "  Warnings: " << s_warningCount << std::endl;
    if (s_errorCount == 0 && !s_fatalFlag)
    {
        std::cerr << "  Status  : OK" << std::endl;
    }
    else
    {
        std::cerr << "  Status  : FAILED" << std::endl;
    }
}