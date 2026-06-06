#include "Interpreter.hpp"
#include <iostream>
#include <fstream>
#include <climits>
#include <sstream>

Interpreter::Interpreter(std::ostream &out) : output(out), stackPtr(-1), basePtr(0), pointerCtr(0), pushCtr(0), popCtr(0) {}

void Interpreter::run(const std::vector<Instruction> &code)
{
    if (code.empty())
    {
        return;
    }

    stackPtr = -1;
    basePtr = 0;
    pointerCtr = 0;
    pushCtr = 0;
    popCtr = 0;
    stack.clear();

    for (const auto &ins : code)
    {
        if (ins.opcode == OpCode::JMP || ins.opcode == OpCode::JPC)
        {
            validateJump(ins.operand, static_cast<int>(code.size()));
        }
    }

    while (pointerCtr >= 0 && pointerCtr < static_cast<int>(code.size()))
    {
        const Instruction &ins = code.at(pointerCtr);

        pointerCtr++;

        if (debugMode)
        {
            std::cerr << "[PC=" << pointerCtr - 1
                      << " BP=" << basePtr
                      << " SP=" << stackPtr << "] "
                      << ICG::opcodeToString(ins.opcode)
                      << " " << ins.level
                      << " " << ins.operand << "\n";
        }

        switch (ins.opcode)
        {
        case OpCode::CAL:
            operateCAL(ins);
            break;
        case OpCode::INT_OP:
            operateINT(ins);
            break;
        case OpCode::LIT:
            operateLIT(ins);
            break;
        case OpCode::LOD:
            operateLOD(ins);
            break;
        case OpCode::OPR:
            operateOPR(ins);
            break;
        case OpCode::RET:
            operateRET(ins, static_cast<int>(code.size()));
            break;
        case OpCode::STO:
            operateSTO(ins);
            break;
        case OpCode::JMP:
            operateJMP(ins, static_cast<int>(code.size()));
            break;
        case OpCode::JPC:
            operateJPC(ins, static_cast<int>(code.size()));
            break;
        case OpCode::LODA:
        {
            int addr = pop().val;
            push(memLoad(addr));
            break;
        }
        case OpCode::STOA:
        {
            int addr = pop().val;
            StackValue val = pop();
            memStore(addr, val);
            break;
        }
        case OpCode::CKB:
        {
            StackValue indexVal = peek();
            if (indexVal.type != DataType::INTEGER && indexVal.type != DataType::CHAR && indexVal.type != DataType::BOOLEAN && indexVal.type != DataType::SUBRANGE)
            {
                throw RuntimeError("Type mismatch: array index must be an ordinal type, got " + SymbolTable::DataTypeToString(indexVal.type));
            }
            int index = indexVal.val;
            int low = ins.level;
            int high = ins.operand;
            if (index < low || index > high)
            {
                throw IndexOutOfBoundsError(index, low, high);
            }
            break;
        }
        }
    }
}

void Interpreter::push(int val, DataType type)
{
    stack.push_back({val, type});
    stackPtr++;
    pushCtr++;
}

void Interpreter::push(StackValue sv)
{
    stack.push_back(sv);
    stackPtr++;
    pushCtr++;
}

StackValue Interpreter::pop()
{
    if (stackPtr < 0 || stack.empty())
    {
        throw StackUnderflowError();
    }

    StackValue val = stack.at(stackPtr);
    stack.pop_back();
    stackPtr--;
    popCtr++;
    return val;
}

StackValue Interpreter::peek() const
{
    if (stackPtr < 0 || stack.empty())
    {
        throw StackUnderflowError();
    }
    return stack.at(stackPtr);
}

int Interpreter::base(int levels, int b) const
{
    while (levels > 0)
    {
        if (b < 0 || b >= static_cast<int>(stack.size()))
            throw StackCorruptionError("static link out of bounds during base() traversal");
        b = stack[b].val; // static link: stack[b+0]
        levels--;
    }
    return b;
}

void Interpreter::validateJump(int target, int codeSize) const
{
    if (target < 0 || target >= codeSize)
        throw InvalidJumpError(target);
}

void Interpreter::pushReal(double dVal)
{
    int idx = -1;
    for (auto &[k, v] : realPool)
    {
        if (v == dVal)
        {
            idx = k;
            break;
        }
    }
    if (idx == -1)
    {
        int minIdx = -1;
        for (auto &[k, v] : realPool)
        {
            if (k < minIdx) minIdx = k;
        }
        idx = minIdx - 1;
        realPool[idx] = dVal;
    }
    push(idx, DataType::REAL);
}

bool Interpreter::compareValues(OprCode op, const StackValue &a, const StackValue &b) const
{
    if ((a.type == DataType::STRING || b.type == DataType::STRING) && a.type != b.type)
    {
        throw RuntimeError("Type mismatch: cannot compare String and " + SymbolTable::DataTypeToString(b.type == DataType::STRING ? a.type : b.type));
    }
    if ((a.type == DataType::BOOLEAN || b.type == DataType::BOOLEAN) && a.type != b.type)
    {
        throw RuntimeError("Type mismatch: cannot compare Boolean and " + SymbolTable::DataTypeToString(b.type == DataType::BOOLEAN ? a.type : b.type));
    }

    if (a.type == DataType::STRING && b.type == DataType::STRING)
    {
        const std::string &sA = stringTable.at(a.val);
        const std::string &sB = stringTable.at(b.val);
        switch (op)
        {
            case OprCode::EQL: return sA == sB;
            case OprCode::NEQ: return sA != sB;
            case OprCode::LSS: return sA < sB;
            case OprCode::GEQ: return sA >= sB;
            case OprCode::GTR: return sA > sB;
            case OprCode::LEQ: return sA <= sB;
            default: return false;
        }
    }
    else if (a.type == DataType::REAL || b.type == DataType::REAL)
    {
        if (a.type != DataType::INTEGER && a.type != DataType::REAL)
            throw RuntimeError("Type mismatch: cannot compare numeric with " + SymbolTable::DataTypeToString(a.type));
        if (b.type != DataType::INTEGER && b.type != DataType::REAL)
            throw RuntimeError("Type mismatch: cannot compare numeric with " + SymbolTable::DataTypeToString(b.type));

        double valA = (a.type == DataType::REAL) ? realPool.at(a.val) : a.val;
        double valB = (b.type == DataType::REAL) ? realPool.at(b.val) : b.val;
        switch (op)
        {
            case OprCode::EQL: return valA == valB;
            case OprCode::NEQ: return valA != valB;
            case OprCode::LSS: return valA < valB;
            case OprCode::GEQ: return valA >= valB;
            case OprCode::GTR: return valA > valB;
            case OprCode::LEQ: return valA <= valB;
            default: return false;
        }
    }
    else
    {
        int valA = a.val;
        int valB = b.val;
        switch (op)
        {
            case OprCode::EQL: return valA == valB;
            case OprCode::NEQ: return valA != valB;
            case OprCode::LSS: return valA < valB;
            case OprCode::GEQ: return valA >= valB;
            case OprCode::GTR: return valA > valB;
            case OprCode::LEQ: return valA <= valB;
            default: return false;
        }
    }
}

StackValue Interpreter::memLoad(int addr) const
{
    if (addr < 0 || addr >= static_cast<int>(stack.size()))
        throw OutOfBoundsError(addr, basePtr, static_cast<int>(stack.size()));
    return stack[addr];
}

void Interpreter::memStore(int addr, StackValue sv)
{
    if (addr < 0 || addr >= static_cast<int>(stack.size()))
        throw OutOfBoundsError(addr, basePtr, static_cast<int>(stack.size()));
    stack[addr] = sv;
}

int Interpreter::doAdd(int a, int b)
{
    long long r = static_cast<long long>(a) + b;
    if (r > INT_MAX_VAL || r < INT_MIN_VAL)
        throw ArithmeticOverflowError("ADD (" + std::to_string(a) + " + " + std::to_string(b) + ")");
    return static_cast<int>(r);
}

int Interpreter::doSub(int a, int b)
{
    long long r = static_cast<long long>(a) - b;
    if (r > INT_MAX_VAL || r < INT_MIN_VAL)
        throw ArithmeticOverflowError("SUB (" + std::to_string(a) + " - " + std::to_string(b) + ")");
    return static_cast<int>(r);
}

int Interpreter::doMul(int a, int b)
{
    long long r = static_cast<long long>(a) * b;
    if (r > INT_MAX_VAL || r < INT_MIN_VAL)
        throw ArithmeticOverflowError("MUL (" + std::to_string(a) + " * " + std::to_string(b) + ")");
    return static_cast<int>(r);
}

int Interpreter::doNeg(int a)
{
    if (a == static_cast<int>(INT_MIN_VAL))
        throw ArithmeticOverflowError("NEG (INT_MIN has no positive counterpart)");
    return -a;
}

void Interpreter::operateLIT(const Instruction &ins)
{
    push(ins.operand, static_cast<DataType>(ins.level));
}

void Interpreter::operateLOD(const Instruction &ins)
{
    int frameBase = base(ins.level, basePtr);
    int addr = frameBase + ins.operand;
    push(memLoad(addr));
}

void Interpreter::operateSTO(const Instruction &instr)
{
    StackValue val = pop();
    int frameBase = base(instr.level, basePtr);
    int addr = frameBase + instr.operand;
    memStore(addr, val);
}

void Interpreter::operateCAL(const Instruction &instr)
{

    int depth = 0;
    int walkBp = basePtr;
    while (walkBp > 0 && depth <= MAX_STACK_DEPTH)
    {
        walkBp = stack[walkBp + 1].val;
        depth++;
    }
    if (depth >= MAX_STACK_DEPTH)
        throw StackOverflowError("Maximum call depth (" + std::to_string(MAX_STACK_DEPTH) + ") exceeded");

    int staticLink = base(instr.level, basePtr);
    int dynamicLink = basePtr;
    int returnAddr = pointerCtr;

    push(staticLink, DataType::INTEGER);
    push(dynamicLink, DataType::INTEGER);
    push(returnAddr, DataType::INTEGER);

    basePtr = stackPtr - 2;
    pointerCtr = instr.operand;
}

void Interpreter::operateINT(const Instruction &instr)
{
    int m = instr.operand;
    if (m < 0)
        throw RuntimeError("INT operand must be non-negative, got " + std::to_string(m));

    int currentFrameSize = stackPtr - basePtr + 1;
    int toAllocate = m - currentFrameSize;

    for (int i = 0; i < toAllocate; i++)
        push(0, DataType::INTEGER);
}

void Interpreter::operateJMP(const Instruction &instr, int codeSize)
{
    validateJump(instr.operand, codeSize);
    pointerCtr = instr.operand;
}

void Interpreter::operateJPC(const Instruction &instr, int codeSize)
{
    StackValue cond = pop();
    if (cond.type != DataType::BOOLEAN && cond.type != DataType::INTEGER)
    {
        throw RuntimeError("Type mismatch: condition must be Boolean or Integer, got " + SymbolTable::DataTypeToString(cond.type));
    }
    if (cond.val == 0)
    {
        validateJump(instr.operand, codeSize);
        pointerCtr = instr.operand;
    }
}

void Interpreter::operateRET(const Instruction &ins, int codeSize)
{
    if (basePtr == 0)
    {
        pointerCtr = -1;
        return;
    }

    if (basePtr < 0 || basePtr + 2 > stackPtr)
    {
        throw StackCorruptionError(
            "bp=" + std::to_string(basePtr) + " is invalid during RET (sp=" + std::to_string(stackPtr) + ")");
    }

    bool isFunction = (ins.level == 1);
    int psze = ins.operand;

    StackValue returnValue;
    if (isFunction)
    {
        if (stackPtr < 0 || stackPtr >= static_cast<int>(stack.size()))
            throw StackCorruptionError("No return value available on stack for function");
        returnValue = stack[stackPtr];
    }

    int retAddr = stack[basePtr + 2].val;
    int callerBp = stack[basePtr + 1].val;

    if (retAddr < 0 || retAddr >= codeSize)
    {
        throw StackSmashingError("Return address corrupted (" + std::to_string(retAddr) + ")");
    }

    if (callerBp < 0 || callerBp >= basePtr)
    {
        throw StackSmashingError("Dynamic link / Caller BP corrupted (" + std::to_string(callerBp) + ")");
    }

    int targetSp = basePtr - psze - 1;

    while (stackPtr > targetSp)
    {
        stack.pop_back();
        stackPtr--;
        popCtr++;
    }

    basePtr = callerBp;
    pointerCtr = retAddr;

    if (isFunction)   
        push(returnValue);
}

void Interpreter::operateOPR(const Instruction &instr)
{
    OprCode op = static_cast<OprCode>(instr.operand);

    switch (op)
    {
    case OprCode::NEG:
    {
        StackValue a = pop();
        if (a.type != DataType::INTEGER && a.type != DataType::REAL)
        {
            throw RuntimeError("Type mismatch: operand of NEG must be numeric, got " + SymbolTable::DataTypeToString(a.type));
        }
        if (a.type == DataType::REAL)
        {
            pushReal(-realPool.at(a.val));
        }
        else
        {
            push(doNeg(a.val), DataType::INTEGER);
        }
        break;
    }
    case OprCode::ADD:
    {
        StackValue b = pop(), a = pop();
        if ((a.type != DataType::INTEGER && a.type != DataType::REAL) ||
            (b.type != DataType::INTEGER && b.type != DataType::REAL))
        {
            throw RuntimeError("Type mismatch: expected numeric operands for ADD, got " + SymbolTable::DataTypeToString(a.type) + " and " + SymbolTable::DataTypeToString(b.type));
        }
        if (a.type == DataType::REAL || b.type == DataType::REAL)
        {
            double valA = (a.type == DataType::REAL) ? realPool.at(a.val) : a.val;
            double valB = (b.type == DataType::REAL) ? realPool.at(b.val) : b.val;
            pushReal(valA + valB);
        }
        else
        {
            push(doAdd(a.val, b.val), DataType::INTEGER);
        }
        break;
    }
    case OprCode::SUB:
    {
        StackValue b = pop(), a = pop();
        if ((a.type != DataType::INTEGER && a.type != DataType::REAL) ||
            (b.type != DataType::INTEGER && b.type != DataType::REAL))
        {
            throw RuntimeError("Type mismatch: expected numeric operands for SUB, got " + SymbolTable::DataTypeToString(a.type) + " and " + SymbolTable::DataTypeToString(b.type));
        }
        if (a.type == DataType::REAL || b.type == DataType::REAL)
        {
            double valA = (a.type == DataType::REAL) ? realPool.at(a.val) : a.val;
            double valB = (b.type == DataType::REAL) ? realPool.at(b.val) : b.val;
            pushReal(valA - valB);
        }
        else
        {
            push(doSub(a.val, b.val), DataType::INTEGER);
        }
        break;
    }
    case OprCode::MUL:
    {
        StackValue b = pop(), a = pop();
        if ((a.type != DataType::INTEGER && a.type != DataType::REAL) ||
            (b.type != DataType::INTEGER && b.type != DataType::REAL))
        {
            throw RuntimeError("Type mismatch: expected numeric operands for MUL, got " + SymbolTable::DataTypeToString(a.type) + " and " + SymbolTable::DataTypeToString(b.type));
        }
        if (a.type == DataType::REAL || b.type == DataType::REAL)
        {
            double valA = (a.type == DataType::REAL) ? realPool.at(a.val) : a.val;
            double valB = (b.type == DataType::REAL) ? realPool.at(b.val) : b.val;
            pushReal(valA * valB);
        }
        else
        {
            push(doMul(a.val, b.val), DataType::INTEGER);
        }
        break;
    }
    case OprCode::DIV:
    {
        StackValue b = pop(), a = pop();
        if ((a.type != DataType::INTEGER && a.type != DataType::REAL) ||
            (b.type != DataType::INTEGER && b.type != DataType::REAL))
        {
            throw RuntimeError("Type mismatch: expected numeric operands for DIV, got " + SymbolTable::DataTypeToString(a.type) + " and " + SymbolTable::DataTypeToString(b.type));
        }
        double valA = (a.type == DataType::REAL) ? realPool.at(a.val) : a.val;
        double valB = (b.type == DataType::REAL) ? realPool.at(b.val) : b.val;
        if (valB == 0.0)
            throw DivisionByZeroError();

        if (a.type == DataType::REAL || b.type == DataType::REAL)
        {
            pushReal(valA / valB);
        }
        else
        {
            if (b.val == -1 && a.val == static_cast<int>(INT_MIN_VAL))
                throw ArithmeticOverflowError("DIV signed overflow (" + std::to_string(a.val) + " / " + std::to_string(b.val) + ")");
            push(a.val / b.val, DataType::INTEGER);
        }
        break;
    }
    case OprCode::MOD:
    {
        StackValue b = pop(), a = pop();
        if (a.type != DataType::INTEGER || b.type != DataType::INTEGER)
        {
            throw RuntimeError("Type mismatch: operands of MOD must be Integer, got " + SymbolTable::DataTypeToString(a.type) + " and " + SymbolTable::DataTypeToString(b.type));
        }
        if (b.val == 0)
            throw DivisionByZeroError();
        if (b.val == -1 && a.val == static_cast<int>(INT_MIN_VAL))
            throw ArithmeticOverflowError("MOD signed overflow (" + std::to_string(a.val) + " % " + std::to_string(b.val) + ")");
        push(a.val % b.val, DataType::INTEGER);
        break;
    }
    case OprCode::EQL:
    case OprCode::NEQ:
    case OprCode::LSS:
    case OprCode::GEQ:
    case OprCode::GTR:
    case OprCode::LEQ:
    {
        StackValue b = pop(), a = pop();
        bool res = compareValues(op, a, b);
        push(res ? 1 : 0, DataType::BOOLEAN);
        break;
    }

    // Output operations
    case OprCode::WRT:
    {
        int val = pop().val;
        outputValue(val, false);
        break;
    }
    case OprCode::WRTLN:
    {
        int val = pop().val;
        outputValue(val, true);
        break;
    }
    case OprCode::PUSHBP:
    {
        push(basePtr, DataType::INTEGER);
        break;
    }   
    case OprCode::RED:
    {
        int addr = pop().val;
        std::string inputStr;
        DataType expectedType = static_cast<DataType>(instr.level);
        bool readSuccess = false;

        if (expectedType == DataType::STRING)
        {
            while (std::cin.peek() == '\n' || std::cin.peek() == '\r')
            {
                std::cin.get();
            }
            if (std::getline(std::cin, inputStr))
            {
                readSuccess = true;
            }
        }
        else
        {
            if (std::cin >> inputStr)
            {
                readSuccess = true;
            }
        }

        if (readSuccess)
        {
            try
            {
                if (expectedType == DataType::REAL)
                {
                    size_t parsedChars = 0;
                    double doubleVal = std::stod(inputStr, &parsedChars);
                    if (parsedChars < inputStr.size())
                    {
                        throw std::invalid_argument("not a pure double");
                    }
                    int idx = -1;
                    for (auto &[k, v] : realPool)
                    {
                        if (v == doubleVal)
                        {
                            idx = k;
                            break;
                        }
                    }
                    if (idx == -1)
                    {
                        int minIdx = -1;
                        for (auto &[k, v] : realPool)
                        {
                            if (k < minIdx) minIdx = k;
                        }
                        idx = minIdx - 1;
                        realPool[idx] = doubleVal;
                    }
                    memStore(addr, {idx, DataType::REAL});
                }
                else if (expectedType == DataType::INTEGER)
                {
                    size_t parsedChars = 0;
                    int intVal = std::stoi(inputStr, &parsedChars);
                    if (parsedChars < inputStr.size())
                    {
                        throw std::invalid_argument("not a pure integer");
                    }
                    memStore(addr, {intVal, DataType::INTEGER});
                }
                else if (expectedType == DataType::CHAR)
                {
                    if (inputStr.size() != 1)
                    {
                        throw std::invalid_argument("not a char");
                    }
                    memStore(addr, {static_cast<int>(inputStr[0]), DataType::CHAR});
                }
                else if (expectedType == DataType::BOOLEAN)
                {
                    std::string lowerStr = inputStr;
                    for (char &c : lowerStr) c = tolower(c);
                    if (lowerStr == "true" || lowerStr == "1")
                    {
                        memStore(addr, {1, DataType::BOOLEAN});
                    }
                    else if (lowerStr == "false" || lowerStr == "0")
                    {
                        memStore(addr, {0, DataType::BOOLEAN});
                    }
                    else
                    {
                        throw std::invalid_argument("not a boolean");
                    }
                }
                else if (expectedType == DataType::STRING)
                {
                    int idx = -1;
                    for (auto &[k, v] : stringTable)
                    {
                        if (v == inputStr)
                        {
                            idx = k;
                            break;
                        }
                    }
                    if (idx == -1)
                    {
                        int minIdx = -100000;
                        for (auto &[k, v] : stringTable)
                        {
                            if (k < minIdx) minIdx = k;
                        }
                        idx = minIdx - 1;
                        stringTable[idx] = inputStr;
                    }
                    memStore(addr, {idx, DataType::STRING});
                }
                else
                {
                    throw std::invalid_argument("unsupported input type");
                }
            }
            catch (...)
            {
                throw RuntimeError("Invalid input: '" + inputStr + "' does not match expected type");
            }
        }
        break;
    }

    default:
        throw RuntimeError("Unknown OPR code: " + std::to_string(instr.operand));
    }
}

std::vector<Instruction> Interpreter::parseFromStream(std::istream &stream)
{
    std::vector<Instruction> code;
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.empty() || line[0] == ';')
            continue;

        auto semiPos = line.find(';');
        if (semiPos != std::string::npos)
            line = line.substr(0, semiPos);

        std::istringstream iss(line);
        int lineNo;
        std::string mnemonic;
        int level, operand;

        if (!(iss >> lineNo >> mnemonic))
            continue;

        if (mnemonic == "RET")
        {
            code.push_back({lineNo, OpCode::RET, 0, 0});
            continue;
        }

        if (!(iss >> level >> operand))
            continue;

        OpCode op;
        if (mnemonic == "LIT")
            op = OpCode::LIT;
        else if (mnemonic == "LOD")
            op = OpCode::LOD;
        else if (mnemonic == "STO")
            op = OpCode::STO;
        else if (mnemonic == "CAL")
            op = OpCode::CAL;
        else if (mnemonic == "INT")
            op = OpCode::INT_OP;
        else if (mnemonic == "JMP")
            op = OpCode::JMP;
        else if (mnemonic == "JPC")
            op = OpCode::JPC;
        else if (mnemonic == "OPR")
            op = OpCode::OPR;
        else if (mnemonic == "LODA")
            op = OpCode::LODA;
        else if (mnemonic == "STOA")
            op = OpCode::STOA;
        else if (mnemonic == "CKB")
            op = OpCode::CKB;
        else
            throw RuntimeError("Unknown mnemonic: " + mnemonic);

        code.push_back({lineNo, op, level, operand});
    }
    return code;
}

std::vector<Instruction> Interpreter::parseFromFile(const std::string &filename)
{
    std::ifstream f(filename);
    if (!f)
        throw RuntimeError("Cannot open file: " + filename);
    return parseFromStream(f);
}

std::vector<Instruction> Interpreter::parseFromString(const std::string &content)
{
    std::istringstream stream(content);
    return parseFromStream(stream);
}

void Interpreter::outputValue(int val, bool newline)
{
    if (val <= -100000 && stringTable.count(val))
    {
        std::string s = stringTable.at(val);
        if (s.size() >= 2 && s.front() == '\'' && s.back() == '\'')
        {
            s = s.substr(1, s.size() - 2);
        }
        size_t pos = 0;
        while ((pos = s.find("''", pos)) != std::string::npos)
        {
            s.replace(pos, 2, "'");
            pos += 1;
        }
        pos = 0;
        while ((pos = s.find("\\n", pos)) != std::string::npos)
        {
            s.replace(pos, 2, "\n");
            pos += 1;
        }
        pos = 0;
        while ((pos = s.find("\\t", pos)) != std::string::npos)
        {
            s.replace(pos, 2, "\t");
            pos += 1;
        }
        output << s;
    }
    else if (val >= -99999 && val <= -1)
    {
        if (realPool.count(val))
        {
            double d = realPool.at(val);
            if (d == (int)d)
            {
                output << d << ".0";
            }
            else
            {
                output << d;
            }
        }
        else
        {
            output << val;
        }
    }
    else
    {
        output << val;
    }

    if (newline)
        output << "\n";
}
