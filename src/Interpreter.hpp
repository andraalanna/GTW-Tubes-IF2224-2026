#pragma once
#include "ICG.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <ostream>

struct RuntimeError : public std::runtime_error
{
    explicit RuntimeError(const std::string &msg)
        : std::runtime_error("[RuntimeError] " + msg) {}
};

struct StackOverflowError : public RuntimeError
{
    explicit StackOverflowError(const std::string &msg = "Stack overflow: maximum frame depth exceeded")
        : RuntimeError(msg) {}
};

struct StackUnderflowError : public RuntimeError
{
    explicit StackUnderflowError(const std::string &msg = "Stack underflow: pop on empty stack")
        : RuntimeError(msg) {}
};

struct StackSmashingError : public RuntimeError
{
    explicit StackSmashingError(const std::string &msg = "Stack smashing: control frame corrupted")
        : RuntimeError(msg) {}
};

struct StackCorruptionError : public RuntimeError
{
    explicit StackCorruptionError(const std::string &msg)
        : RuntimeError("Stack corruption: " + msg) {}
};

struct InvalidJumpError : public RuntimeError
{
    explicit InvalidJumpError(int target)
        : RuntimeError("Invalid jump target: line " + std::to_string(target)) {}
};

struct OutOfBoundsError : public RuntimeError
{
    explicit OutOfBoundsError(int addr, int base, int limit)
        : RuntimeError("Memory access out of bounds: address " + std::to_string(addr) + " (frame base=" + std::to_string(base) + ", limit=" + std::to_string(limit) + ")") {}
};

struct IndexOutOfBoundsError : public RuntimeError
{
    explicit IndexOutOfBoundsError(int index, int low, int high)
        : RuntimeError("Index out of bounds: index " + std::to_string(index) + " is not in range [" + std::to_string(low) + ".." + std::to_string(high) + "]") {}
};

struct DivisionByZeroError : public RuntimeError
{
    explicit DivisionByZeroError()
        : RuntimeError("Division by zero") {}
};

struct ArithmeticOverflowError : public RuntimeError
{
    explicit ArithmeticOverflowError(const std::string &op)
        : RuntimeError("Arithmetic overflow in operation: " + op) {}
};

class Interpreter {
    public:
        static constexpr int MAX_STACK_DEPTH = 1000;
        static constexpr long long INT_MAX_VAL = 2147483647LL;
        static constexpr long long INT_MIN_VAL = -2147483648LL;

        explicit Interpreter(std::ostream &out = std::cout);

        void run(const std::vector<Instruction> &code);
        std::unordered_map<int, std::string> stringTable;
        std::unordered_map<int, double> realPool;

    private:
        std::ostream& output;
        std::vector<int> stack;
        int stackPtr;
        int basePtr;
        int pointerCtr;
        int pushCtr;
        int popCtr;
        void push(int val);
        int pop();
        int peek() const;
        bool debugMode = false;

        int base(int levels, int b) const;
        void outputValue(int val, bool newline);

        int memLoad(int addr) const;
        void memStore(int addr, int val);

        void validateJump(int target, int codeSize) const;

        int doAdd(int a, int b);
        int doSub(int a, int b);
        int doMul(int a, int b);
        int doNeg(int a);

        void operateLIT(const Instruction &instr);
        void operateLOD(const Instruction &instr);
        void operateSTO(const Instruction &instr);
        void operateCAL(const Instruction &instr);
        void operateINT(const Instruction &instr);
        void operateJMP(const Instruction &instr, int codeSize);
        void operateJPC(const Instruction &instr, int codeSize);
        void operateOPR(const Instruction &instr);
        void operateRET(const Instruction &instr, int codeSize);

        static std::vector<Instruction> parseFromStream(std::istream &stream);
        static std::vector<Instruction> parseFromFile(const std::string &filename);
        static std::vector<Instruction> parseFromString(const std::string &content);
};