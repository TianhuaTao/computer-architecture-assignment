//
// Created by Sam on 2020/5/11.
//

#include "Instruction.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

// factory function
Instruction::Operand Instruction::Operand::Register(int id) {
    return Instruction::Operand{true, id};
}

// factory function
Instruction::Operand Instruction::Operand::Immediate(int value) {
    return Instruction::Operand{false, value};
}

std::vector<Instruction> Instruction::getInstructionQueue(const std::string &filename) {

    std::ifstream inStream(filename);

    if (!inStream.is_open()) {
        std::cerr << "Can not open " << filename << std::endl;
        return std::vector<Instruction>();
    }
    std::vector<Instruction> result;

    while (inStream) {
        std::string line, buffer;
        getline(inStream, line); // read in one line
        if (line.length() == 0)
            break;

        // split by ','
        istringstream iss(line);
        vector<string> split;
        while (getline(iss, buffer, ',')) {
            split.push_back(buffer);
        }

        assert(split.size() >= 3);
        Instruction instruction;
        if (split[0] == "LD") {
            instruction.operation = Operation::LD;
            if (split[1][0] != 'R') {
                cerr << "Need a register: " << line << endl;
            }
            split[1] = string(split[1].begin() + 1, split[1].end());
            instruction.operands[0] = Operand::Register(std::stoi(split[1]));

            int im = (int)std::stoll(split[2], nullptr, 16);
            instruction.operands[1] = Operand::Immediate(im);
        } else if (split[0] == "JUMP") {
            instruction.operation = Operation::JUMP;
            if (split[1][0] != 'R' && split[2][0] == 'R' && split[3][0] != 'R') {
                int s1, s2;
                s1 = static_cast<int>(stoll(split[1], nullptr, 16));
                s2 = static_cast<int>(stoll(split[3], nullptr, 16));
                instruction.operands[0] = Operand::Immediate(s1);
                split[2] = string(split[2].begin() + 1, split[2].end());
                instruction.operands[1] = Operand::Register(std::stoi(split[2]));
                instruction.operands[2] = Operand::Immediate(s2);
            } else {
                cerr << "Syntax error: " << line << endl;
            }
        } else {
            if (split[0] == "ADD") {
                instruction.operation = Operation::ADD;
            } else if (split[0] == "SUB") {
                instruction.operation = Operation::SUB;
            } else if (split[0] == "MUL") {
                instruction.operation = Operation::MUL;
            } else if (split[0] == "DIV") {
                instruction.operation = Operation::DIV;
            } else {
                cerr << "Unexpected: " << split[0] << std::endl;
            }

            if (split[1][0] == 'R' && split[2][0] == 'R' && split[3][0] == 'R') {
                for (int i = 0; i < 3; ++i) {
                    split[i + 1] = string(split[i + 1].begin() + 1, split[i + 1].end());
                    instruction.operands[i] = Operand::Register(std::stoi(split[i + 1]));
                }
            } else {
                cerr << "Need a register: " << line << endl;
            }
        }
        result.push_back(instruction);
    }

    return result;
}
