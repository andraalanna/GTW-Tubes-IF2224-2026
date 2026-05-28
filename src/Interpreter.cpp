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
            operateRET();
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
            int addr = pop();
            push(memLoad(addr));
            break;
        }
        case OpCode::STOA:
        {
            int addr = pop();
            int val = pop();
            memStore(addr, val);
            break;
        }
        }
    }
}

void Interpreter::push(int val)
{
    stack.push_back(val);
    stackPtr++;
    pushCtr++;
}

int Interpreter::pop()
{
    if (stackPtr < 0 || stack.empty())
    {
        throw StackUnderflowError();
    }

    int val = stack.at(stackPtr);
    stack.pop_back();
    stackPtr--;
    popCtr++;
    return val;
}

int Interpreter::peek() const
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
        b = stack[b]; // static link: stack[b+0]
        levels--;
    }
    return b;
}

void Interpreter::validateJump(int target, int codeSize) const
{
    if (target < 0 || target >= codeSize)
        throw InvalidJumpError(target);
}

int Interpreter::memLoad(int addr) const
{
    if (addr < 0 || addr >= static_cast<int>(stack.size()))
        throw OutOfBoundsError(addr, basePtr, static_cast<int>(stack.size()));
    return stack[addr];
}

void Interpreter::memStore(int addr, int val)
{
    if (addr < 0 || addr >= static_cast<int>(stack.size()))
        throw OutOfBoundsError(addr, basePtr, static_cast<int>(stack.size()));
    stack[addr] = val;
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
    push(ins.operand);
}

void Interpreter::operateLOD(const Instruction &ins)
{
    int frameBase = base(ins.level, basePtr);
    int addr = frameBase + ins.operand;
    push(memLoad(addr));
}

void Interpreter::operateSTO(const Instruction &instr)
{
    int val = pop();
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
        walkBp = stack[walkBp + 1];
        depth++;
    }
    if (depth >= MAX_STACK_DEPTH)
        throw StackOverflowError("Maximum call depth (" + std::to_string(MAX_STACK_DEPTH) + ") exceeded");

    int staticLink = base(instr.level, basePtr);
    int dynamicLink = basePtr;
    int returnAddr = pointerCtr;

    push(staticLink);
    push(dynamicLink);
    push(returnAddr);

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
        push(0);
}

void Interpreter::operateJMP(const Instruction &instr, int codeSize)
{
    validateJump(instr.operand, codeSize);
    pointerCtr = instr.operand;
}

void Interpreter::operateJPC(const Instruction &instr, int codeSize)
{
    int cond = pop();
    if (cond == 0)
    {
        validateJump(instr.operand, codeSize);
        pointerCtr = instr.operand;
    }
}

void Interpreter::operateRET()
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

    bool hasReturnValue = (stackPtr > basePtr + 2);
    int returnValue = hasReturnValue ? stack[stackPtr] : 0;

    int retAddr = stack[basePtr + 2];  // return address
    int callerBp = stack[basePtr + 1]; // dynamic link

    while (stackPtr >= basePtr)
    {
        stack.pop_back();
        stackPtr--;
        popCtr++;
    }

    basePtr = callerBp;
    pointerCtr = retAddr;

    if (hasReturnValue)   
        push(returnValue);
}

void Interpreter::operateOPR(const Instruction &instr)
{
    OprCode op = static_cast<OprCode>(instr.operand);

    switch (op)
    {
    case OprCode::NEG:
    {
        int a = pop();
        push(doNeg(a));
        break;
    }
    case OprCode::ADD:
    {
        int b = pop(), a = pop();
        push(doAdd(a, b));
        break;
    }
    case OprCode::SUB:
    {
        int b = pop(), a = pop();
        push(doSub(a, b));
        break;
    }
    case OprCode::MUL:
    {
        int b = pop(), a = pop();
        push(doMul(a, b));
        break;
    }
    case OprCode::DIV:
    {
        int b = pop(), a = pop();
        if (b == 0)
            throw DivisionByZeroError();
        push(a / b);
        break;
    }
    case OprCode::MOD:
    {
        int b = pop(), a = pop();
        if (b == 0)
            throw DivisionByZeroError();
        push(a % b);
        break;
    }
    case OprCode::EQL:
    {
        int b = pop(), a = pop();
        push(a == b ? 1 : 0);
        break;
    }
    case OprCode::NEQ:
    {
        int b = pop(), a = pop();
        push(a != b ? 1 : 0);
        break;
    }
    case OprCode::LSS:
    {
        int b = pop(), a = pop();
        push(a < b ? 1 : 0);
        break;
    }
    case OprCode::GEQ:
    {
        int b = pop(), a = pop();
        push(a >= b ? 1 : 0);
        break;
    }
    case OprCode::GTR:
    {
        int b = pop(), a = pop();
        push(a > b ? 1 : 0);
        break;
    }
    case OprCode::LEQ:
    {
        int b = pop(), a = pop();
        push(a <= b ? 1 : 0);
        break;
    }

    // Output operations
    case OprCode::WRT:
    {
        int val = pop();
        output << val;
        break;
    }
    case OprCode::WRTLN:
    {
        int val = pop();
        output << val << "\n";
        break;
    }
    case OprCode::PUSHBP:
    {
        push(basePtr);
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
