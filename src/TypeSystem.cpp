#include "TypeSystem.h"

std::string dataTypeToString(DataType t)
{
    return SymbolTable::DataTypeToString(t);
}

bool isSimpleType(DataType t)
{
    return (t == DataType::INTEGER ||
            t == DataType::REAL ||
            t == DataType::BOOLEAN ||
            t == DataType::CHAR ||
            t == DataType::STRING ||
            t == DataType::SUBRANGE ||
            t == DataType::ENUMERATED);
}

bool isValidIndexType(DataType t)
{
    // Real dilarang sebagai index type
    if (t == DataType::REAL)
        return false;
    return (t == DataType::INTEGER ||
            t == DataType::BOOLEAN ||
            t == DataType::CHAR ||
            t == DataType::SUBRANGE ||
            t == DataType::ENUMERATED);
}

bool isCompatible(DataType t1, DataType t2, int t1ref, int t2ref)
{
    if (t1 == DataType::UNKNOWN || t2 == DataType::UNKNOWN)
        return true;

    if (t1 == t2)
    {
        if (t1 == DataType::RECORD)
        {
            return (t1ref != 0 && t1ref == t2ref);
        }
        if (t1 == DataType::ARRAY)
        {
            return (t1ref != 0 && t1ref == t2ref);
        }
        return true;
    }

    if (t1 == DataType::SUBRANGE || t2 == DataType::SUBRANGE)
    {
        DataType other = (t1 == DataType::SUBRANGE) ? t2 : t1;
        if (other == DataType::REAL)
            return false;
        return (other == DataType::INTEGER ||
                other == DataType::CHAR ||
                other == DataType::BOOLEAN ||
                other == DataType::SUBRANGE);
    }

    if (t1 == DataType::STRING && t2 == DataType::STRING)
    {
        return true;
    }

    return false;
}

bool isAssignCompatible(DataType t1, DataType t2, int t1ref, int t2ref)
{
    if (t1 == DataType::UNKNOWN || t2 == DataType::UNKNOWN)
        return true;
    if (t1 == DataType::REAL && t2 == DataType::INTEGER)
        return true;

    if (t1 == t2)
    {
        if (t1 == DataType::ARRAY)
            return (t1ref != 0 && t1ref == t2ref);
        if (t1 == DataType::RECORD)
            return (t1ref != 0 && t1ref == t2ref);
        return true;
    }

    if (isCompatible(t1, t2, t1ref, t2ref))
    {
        if (t1 == DataType::INTEGER || t1 == DataType::BOOLEAN ||
            t1 == DataType::CHAR || t1 == DataType::SUBRANGE)
        {
            return true;
        }
    }

    if (t1 == DataType::STRING && t2 == DataType::STRING)
    {
        return isCompatible(t1, t2, t1ref, t2ref);
    }

    return false;
}

DataType inferBinOpType(const std::string &op, DataType leftType, DataType rightType)
{
    if (leftType == DataType::UNKNOWN || rightType == DataType::UNKNOWN)
    {
        return DataType::UNKNOWN;
    }

    if (op == "rdiv")
    {
        if ((leftType == DataType::INTEGER || leftType == DataType::REAL) &&
            (rightType == DataType::INTEGER || rightType == DataType::REAL))
        {
            return DataType::REAL;
        }
        return DataType::UNKNOWN;
    }

    if (op == "idiv" || op == "imod")
    {
        if (leftType == DataType::INTEGER && rightType == DataType::INTEGER)
        {
            return DataType::INTEGER;
        }
        return DataType::UNKNOWN;
    }

    if (op == "plus" || op == "minus" || op == "times")
    {
        if (leftType == DataType::INTEGER && rightType == DataType::INTEGER)
        {
            return DataType::INTEGER;
        }
        if (leftType == DataType::REAL && rightType == DataType::REAL)
        {
            return DataType::REAL;
        }
        if ((leftType == DataType::INTEGER && rightType == DataType::REAL) ||
            (leftType == DataType::REAL && rightType == DataType::INTEGER))
        {
            return DataType::REAL;
        }
        return DataType::UNKNOWN;
    }

    if (op == "andsy" || op == "orsy")
    {
        if (leftType == DataType::BOOLEAN && rightType == DataType::BOOLEAN)
        {
            return DataType::BOOLEAN;
        }
        return DataType::UNKNOWN;
    }

    if (op == "eql" || op == "neq" ||
        op == "lss" || op == "leq" ||
        op == "gtr" || op == "geq")
    {
        auto isRelationalType = [](DataType t)
        {
            return (t == DataType::INTEGER ||
                    t == DataType::REAL ||
                    t == DataType::CHAR ||
                    t == DataType::STRING ||
                    t == DataType::BOOLEAN ||
                    t == DataType::SUBRANGE ||
                    t == DataType::ENUMERATED);
        };

        if (!isRelationalType(leftType) || !isRelationalType(rightType))
        {
            return DataType::UNKNOWN;
        }

        if (leftType == rightType)
            return DataType::BOOLEAN;

        if ((leftType == DataType::INTEGER && rightType == DataType::REAL) ||
            (leftType == DataType::REAL && rightType == DataType::INTEGER))
        {
            return DataType::BOOLEAN;
        }
        if (isCompatible(leftType, rightType))
            return DataType::BOOLEAN;

        return DataType::UNKNOWN;
    }
    return DataType::UNKNOWN;
}

DataType inferUnaryOpType(const std::string &op, DataType operandType)
{
    if (operandType == DataType::UNKNOWN)
        return DataType::UNKNOWN;

    if (op == "notsy")
    {
        if (operandType == DataType::BOOLEAN)
            return DataType::BOOLEAN;
        return DataType::UNKNOWN;
    }

    if (op == "plus" || op == "minus")
    {
        if (operandType == DataType::INTEGER)
            return DataType::INTEGER;
        if (operandType == DataType::REAL)
            return DataType::REAL;
        return DataType::UNKNOWN;
    }

    return DataType::UNKNOWN;
}