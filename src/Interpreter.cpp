#include "Interpreter.hpp"
#include <iostream>
#include <climits>
#include <sstream>

Interpreter::Interpreter(std::ostream &out ): output(out), stackPtr(-1), basePtr(0), pointerCtr(0), pushCtr(0), popCtr(0){}

void Interpreter::run(const std::vector<Instruction> &code)
{   
    if(code.empty()){
            return;
    }

    stackPtr = -1;
    basePtr = 0;
    pointerCtr = 0;
    pushCtr = 0;
    popCtr = 0;
    stack.clear();

    for(const auto& ins: code){
        if(ins.opcode == OpCode::JMP || ins.opcode == OpCode::JPC){
            validateJump(ins.operand, static_cast<int>(code.size()));
        }
    }

    while(pointerCtr >= 0 && pointerCtr < static_cast<int>(code.size())){
        const Instruction& ins = code.at(pointerCtr);

        pointerCtr++;

        switch(ins.opcode){
            case OpCode::CAL: operateCAL(ins); break;
            case OpCode::INT_OP: operateINT(ins);
                break;
            case OpCode::LIT: operateLIT(ins);
                break;
            case OpCode::LOD: operateLOD(ins); break;
            case OpCode::OPR: operateOPR(ins); break;
            case OpCode::RET: operateRET(); break;
            case OpCode::STO: operateSTO(ins); break;
            case OpCode::JMP: operateJMP(ins, static_cast<int>(code.size())); break;
            case OpCode::JPC: operateJPC(ins, static_cast<int>(code.size()));break;    
        }
    }
}

void Interpreter::push(int val){
    stack.push_back(val);
    stackPtr++;
    pushCtr++;
}

int Interpreter::pop(){
    if(stackPtr < 0 || stack.empty()){
        throw StackUnderflowError();
    }

    int val = stack.at(stackPtr);
    stack.pop_back();
    stackPtr--;
    popCtr++;
    return val;
}

int Interpreter::peek() const{
    if(stackPtr < 0 || stack.empty()){
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

// INT 0 m  →  allocate m slots on stack (grow stack for this frame's variables)
//   The first 3 slots were pushed by CAL (or are the bootstrap frame).
//   For the main program, we simply allocate m slots from scratch.
void Interpreter::operateINT(const Instruction &instr)
{
    int m = instr.operand;
    if (m < 0)
        throw RuntimeError("INT operand must be non-negative, got " + std::to_string(m));

    // If this is the very first INT (main program), bootstrap bp and push m slots.
    // Otherwise, we may already have the 3 link slots; push remaining locals.
    // Convention: CAL pushes 3 link slots; INT then pushes (m - 3) additional slots
    // to reach total frame size m.  For the root frame (no CAL), INT pushes m slots.
    int currentFrameSize = stackPtr - basePtr + 1;
    int toAllocate = m - currentFrameSize;

    for (int i = 0; i < toAllocate; i++)
        push(0); // initialize locals to 0
}

// JMP 0 target  →  unconditional jump
void Interpreter::operateJMP(const Instruction &instr, int codeSize)
{
    validateJump(instr.operand, codeSize);
    pointerCtr = instr.operand;
}

// JPC 0 target  →  conditional jump: jump if top of stack == 0 (false)
void Interpreter::operateJPC(const Instruction &instr, int codeSize)
{
    int cond = pop();
    if (cond == 0)
    {
        validateJump(instr.operand, codeSize);
        pointerCtr = instr.operand;
    }
}

// RET  →  return from subroutine; restore caller's context
//   On return:
//     new sp = bp - 1          (discard entire frame)
//     pc     = stack[bp + 2]   (return address)
//     bp     = stack[bp + 1]   (dynamic link = caller's bp)
void Interpreter::operateRET()
{
    if (basePtr == 0 && stackPtr <= 0)
    {
        // Returning from main program — halt
        pointerCtr = -1;
        return;
    }

    // Bonus: Stack Corruption guard — verify bp is within stack
    if (basePtr < 0 || basePtr + 2 > stackPtr)
    {
        throw StackCorruptionError(
            "bp=" + std::to_string(basePtr) + " is invalid during RET (sp=" + std::to_string(stackPtr) + ")");
    }

    int retAddr = stack[basePtr + 2];  // return address
    int callerBp = stack[basePtr + 1]; // dynamic link

    // Pop the entire frame
    while (stackPtr >= basePtr)
    {
        stack.pop_back();
        stackPtr--;
        popCtr++;
    }

    basePtr = callerBp;
    pointerCtr = retAddr;
}

// OPR 0 op  →  execute operation 'op' on stack values
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
    // Comparisons — push 1 (true) or 0 (false)
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

    default:
        throw RuntimeError("Unknown OPR code: " + std::to_string(instr.operand));
    }
}