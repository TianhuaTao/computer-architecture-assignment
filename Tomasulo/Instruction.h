//
// Created by Sam on 2020/5/11.
//

#ifndef TOMASULO_INSTRUCTION_H
#define TOMASULO_INSTRUCTION_H

#include <cstdlib>
#include <iostream>
#include <queue>
#include <string>
#include <vector>
class Instruction {
public:
    enum class Operation{
        NOP = 0, // empty operation
        ADD,SUB,MUL,DIV,LD,
        JUMP
    };
    class Operand{
    public:
        explicit Operand (bool isReg= false, long long v=0):_isRegister(isReg),value(v){}
        static Operand Register(long long id);
        static Operand Immediate(long long value);
        bool isRegister() const{ return _isRegister;}
        bool isImmediate() const{ return !_isRegister;}
        long long getValue() const { return value;}
    private:
        bool _isRegister;
        long long value;
    };

    static std::vector<Instruction> getInstructionQueue(const std::string& filename);

    Operation operation = Operation::NOP;
    Operand operands[3];

private:
};


#endif //TOMASULO_INSTRUCTION_H
