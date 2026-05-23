#pragma once

#include "symbolTable.hpp"
#include <string>

bool isSimpleType(DataType t);

bool isValidIndexType(DataType t);

bool isCompatible(DataType t1, DataType t2, int t1ref = 0, int t2ref = 0);

bool isAssignCompatible(DataType t1, DataType t2, int t1ref = 0, int t2ref = 0);

DataType inferBinOpType(const std::string &op, DataType leftType, DataType rightType);

DataType inferUnaryOpType(const std::string &op, DataType operandType);

std::string dataTypeToString(DataType t);