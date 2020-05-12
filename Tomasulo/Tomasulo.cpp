//
// Created by Sam on 2020/5/11.
//

#include "Tomasulo.h"
#include <vector>

// TODO: convert long long to int
void Tomasulo::run(bool step) {
    while (true) {
        bool finished = instructionQueue.size() == pc &&
                        reservationStation.empty() &&
                        loadBuffer.empty() &&
                        arithmeticComponentStatus.empty();

        if (finished) {
            assert(registerStatus.empty());
            logger.printSummary();
            return;
        }

        /* cycle start */

        // Write Result (of last cycle)
        for (int j = 0; j < ArithmeticComponentStatus::TOTAL_COMPONENT_COUNT; ++j) {
            if (arithmeticComponentStatus.op[j] != Instruction::Operation::NOP) {   // working
                if (arithmeticComponentStatus.cyclesRemain[j] == 0) {   // complete
                    if (arithmeticComponentStatus.op[j] == Instruction::Operation::JUMP) {
                        long long target = arithmeticComponentStatus.RS[j]; // broadcast target
                        // reset reservation station
                        reservationStation.busy[(size_t) target & MASK_LOW] = false;
                        // reset the Arithmetic Component
                        arithmeticComponentStatus.op[j] = Instruction::Operation::NOP;
                        jumpIssued = false;
                        logger.logWriteResult(arithmeticComponentStatus.pc[j]);
                        auto s1 = arithmeticComponentStatus.input0[j];
                        auto s2 = arithmeticComponentStatus.input1[j];
                        if (s1 == s2) {
                            pc += jumpDiff;
                            pc--;   // JUMP itself
                        }
                    } else {
                        long long target = arithmeticComponentStatus.RS[j]; // broadcast target

                        // calculate broadcast value
                        long long value = 0;
                        auto operation = arithmeticComponentStatus.op[j];
                        auto s1 = arithmeticComponentStatus.input0[j];
                        auto s2 = arithmeticComponentStatus.input1[j];
                        if (operation == Instruction::Operation::ADD) {
                            value = s1 + s2;
                        } else if (operation == Instruction::Operation::SUB) {
                            value = s1 - s2;
                        } else if (operation == Instruction::Operation::MUL) {
                            value = s1 * s2;
                        } else if (operation == Instruction::Operation::DIV) {
                            if (s2)
                                value = s1 / s2;
                            else
                                value = s1;
                        } else if (operation == Instruction::Operation::LD) {
                            value = s1;
                        } else {
                            assert(false);
                        }

                        // reset reservation station
                        if (arithmeticComponentStatus.op[j] ==
                            Instruction::Operation::LD) // reset reservation station or load buffer
                        {
                            loadBuffer.busy[(size_t) target & MASK_LOW] = false;
                        } else {
                            reservationStation.busy[(size_t) target & MASK_LOW] = false;
                        }

                        // reset the Arithmetic Component
                        arithmeticComponentStatus.op[j] = Instruction::Operation::NOP;

                        // broadcast to RS, Registers
                        reservationStation.receiveBroadcast(target, value);
                        registerStatus.receiveBroadcast(target, value);
                        /* loadBuffer.receiveBroadcast(target, value); */   // loadBuffer doesn't need to receive broadcast

                        logger.logWriteResult(arithmeticComponentStatus.pc[j]);
                    }
                }
            }
        }
        // Issue
        if (pc < instructionQueue.size() && !jumpIssued) {
            Instruction ins = instructionQueue[pc];
            long long id;
            switch (ins.operation) {
                case Instruction::Operation::LD:
                    id = loadBuffer.getFreeBuffer();
                    if (id != -1) {
                        loadBuffer.use(id, ins.operands[1].getValue(), pc); // operands[1] is value
                        registerStatus.setTag(ins.operands[0].getValue(),
                                              (long long) Tomasulo::LoadBuffer::getHashTag(id));// operands[0] is value
                        logger.logIssue(pc);
                        pc++;

                    }

                    break;
                case Instruction::Operation::ADD:
                case Instruction::Operation::SUB:
                case Instruction::Operation::MUL:
                case Instruction::Operation::DIV:
                    id = reservationStation.getFreeStation(ins.operation);
                    if (id != -1) {
                        reservationStation.use(id, ins, registerStatus, pc);
                        logger.logIssue(pc);
                        pc++;
                    }
                    break;
                case Instruction::Operation::JUMP:
                    id = reservationStation.getFreeStation(ins.operation);
                    if (id != -1) {
                        reservationStation.use(id, ins, registerStatus, pc);
                        jumpIssued = true;
                        jumpDiff = ins.operands[2].getValue();
                        logger.logIssue(pc);
                        pc++;
                    }
                    break;
                default:
                    std::cerr << "Unknown instruction" << std::endl;
                    break;
            }
        }

        // Execute
        for (int j = 0; j < ArithmeticComponentStatus::TOTAL_COMPONENT_COUNT; ++j) {
            if (arithmeticComponentStatus.op[j] != Instruction::Operation::NOP) {   // working
                // no component should be ready, since checked before during Write Result
                assert(arithmeticComponentStatus.cyclesRemain[j]!=0);
                arithmeticComponentStatus.cyclesRemain[j]--;
                if(arithmeticComponentStatus.cyclesRemain[j]==0){
                    logger.logExecuteComplete(arithmeticComponentStatus.pc[j]);
                }
            }
        }
        // RS update, if ready, push into ALU
        // TODO: sort RS ready items by time
        for (int jj = 0; jj < ReservationStation::TOTAL_STATION_COUNT; ++jj) {
            if (reservationStation.busy[jj] && reservationStation.tagSource0[jj] == 0 &&
                reservationStation.tagSource1[jj] == 0 && !reservationStation.executing[jj]) {   // ready


                // if there is empty ALU, start executing
                auto operation = reservationStation.op[jj];
                long long compId = -1;
                if (operation == Instruction::Operation::ADD || operation == Instruction::Operation::SUB ||
                    operation == Instruction::Operation::JUMP) {
                    compId = arithmeticComponentStatus.getFreeAddComponent();
                } else if (operation == Instruction::Operation::MUL || operation == Instruction::Operation::DIV) {
                    compId = arithmeticComponentStatus.getFreeMultComponent();
                }
                if (compId != -1) { // free unit
                    arithmeticComponentStatus.use(compId, (long long) ReservationStation::getHashTag(jj), operation,
                                                  reservationStation.source0[jj], reservationStation.source1[jj], reservationStation.pc[jj]);
                    reservationStation.executing[jj] = true;
                }
            }
        }
        // check Load Buffer
        auto loadComponentId = arithmeticComponentStatus.getFreeLoadComponent();
        if (loadComponentId != -1) {
            std::vector<int> readyLB;
            for (int i = 0; i < NUMBER_OF_LOAD_BUFFER; ++i) {
                if (loadBuffer.busy[i] && !loadBuffer.executing[i])
                    readyLB.push_back(i);
            }

            if (!readyLB.empty()) {
                // TODO: sort and get the first
                int buffer_id = readyLB[0];
                arithmeticComponentStatus.use(loadComponentId, (long long) LoadBuffer::getHashTag(buffer_id),
                                              Instruction::Operation::LD, loadBuffer.value[buffer_id], 0, loadBuffer.pc[buffer_id]);
                loadBuffer.executing[buffer_id] = true;
            }
        }

        logger.printStatus();
        cycleCount++;
        /* cycle end */
    }
}

size_t Tomasulo::getCycleCount() const {
    return cycleCount;
}

bool Tomasulo::ReservationStation::empty() const {
    for (auto i : busy) {
        if (i)return false;
    }
    return true;
}

void Tomasulo::ReservationStation::receiveBroadcast(long long target, long long value) {
    for (int i = 0; i < TOTAL_STATION_COUNT; ++i) {
        if (tagSource0[i] == target) {
            source0[i] = value;
            tagSource0[i] = 0;
        }
        if (tagSource1[i] == target) {
            source1[i] = value;
            tagSource1[i] = 0;
        }
    }
}

long long Tomasulo::ReservationStation::getFreeStation(Instruction::Operation next) const {
    if (next == Instruction::Operation::ADD || next == Instruction::Operation::SUB ||
        next == Instruction::Operation::JUMP) {
        for (int i = ADD_BEGIN; i < ADD_BEGIN + NUMBER_OF_ADD_RESERVATION; ++i) {
            if (!busy[i]) {
                return i;
            }
        }
        return -1;
    } else if (next == Instruction::Operation::MUL || next == Instruction::Operation::DIV) {
        for (int i = MULT_BEGIN; i < MULT_BEGIN + NUMBER_OF_MULTIPLY_RESERVATION; ++i) {
            if (!busy[i]) {
                return i;
            }
        }
        return -1;
    } else {
        assert(false);
        return -1;
    }
}

void Tomasulo::ReservationStation::use(long long id, const Instruction &ins, RegisterStatus &regStatus, size_t inc_pc) {
    assert(!busy[id]);
    executing[id] = false; // reset
    pc[id] = inc_pc;
    if (ins.operation == Instruction::Operation::JUMP) {
        assert(ins.operands[0].isImmediate() && ins.operands[1].isRegister() &&
               ins.operands[2].isImmediate());
        op[id] = ins.operation;
        busy[id] = true;

        // src0
        long long sourceReg0 = ins.operands[1].getValue();
        if (regStatus.tag[sourceReg0] == 0) {  // is immediate value
            source0[id] = regStatus.value[sourceReg0];
            tagSource0[id] = 0;
        } else {
            tagSource0[id] = regStatus.value[sourceReg0];
        }
        // src 1, immediate value
        // need to convert from 64 bit to 32 bit
        source1[id] = static_cast<long long>(((size_t) ins.operands[0].getValue()) &
                                             MASK_32_BIT); // to get rid of compiler warning
        tagSource1[id] = false;

    } else {
        assert(ins.operands[0].isRegister() && ins.operands[1].isRegister() &&
               ins.operands[2].isRegister());   // all registers
        op[id] = ins.operation;
        busy[id] = true;
        long long sinkReg, sourceReg0, sourceReg1;
        sinkReg = ins.operands[0].getValue();
        sourceReg0 = ins.operands[1].getValue();
        sourceReg1 = ins.operands[2].getValue();

        // src0
        if (regStatus.tag[sourceReg0] == 0) {  // is immediate value
            source0[id] = regStatus.value[sourceReg0];
            tagSource0[id] = 0;
        } else {
            tagSource0[id] = regStatus.value[sourceReg0];
        }

        // src1
        if (regStatus.tag[sourceReg1] == 0) {  // is immediate value
            source1[id] = regStatus.value[sourceReg1];
            tagSource1[id] = 0;
        } else {
            tagSource1[id] = regStatus.value[sourceReg1];
        }

        // sink
        regStatus.tag[sinkReg] = true;
        regStatus.value[sinkReg] = (long long) getHashTag(id);
    }

}

void Tomasulo::ReservationStation::clear() {
    for (auto &x:busy) {
        x = false;
    }
    for (auto &x:op) {
        x = Instruction::Operation::NOP;
    }
    for (auto &x:tagSource0) {
        x = 0;
    }
    for (auto &x:tagSource1) {
        x = 0;
    }
    for (auto &x:source0) {
        x = 0;
    }
    for (auto &x:source1) {
        x = 0;
    }
    for (auto &x:executing) {
        x = false;
    }
}

void Tomasulo::RegisterStatus::receiveBroadcast(long long target, long long v) {
    for (int i = 0; i < MAX_REGISTER_ALLOWED; ++i) {
        if (tag[i] && value[i] == target) {
            tag[i] = false;
            value[i] = v;
        }
    }
}

void Tomasulo::RegisterStatus::clear() {
    for (auto &x: tag) x = false;
    for (auto &x: value) x = 0;
}

bool Tomasulo::RegisterStatus::empty() const {
    for (bool i: tag) { if (i)return false; }
    return true;
}

bool Tomasulo::ArithmeticComponentStatus::empty() const {
    for (auto i : tag) {
        if (i != 0) return false;
    }
    return true;
}

long long Tomasulo::ArithmeticComponentStatus::getFreeLoadComponent() const {
    for (int i = Tomasulo::ArithmeticComponentStatus::LOAD_BEGIN;
         i < Tomasulo::ArithmeticComponentStatus::LOAD_BEGIN + NUMBER_OF_LOAD_COMPONENT; ++i) {
        if (op[i] == Instruction::Operation::NOP)return i;
    }
    return -1;
}

long long Tomasulo::ArithmeticComponentStatus::getFreeMultComponent() const {
    for (int i = Tomasulo::ArithmeticComponentStatus::MULT_BEGIN;
         i < Tomasulo::ArithmeticComponentStatus::MULT_BEGIN + NUMBER_OF_MULTIPLY_COMPONENT; ++i) {
        if (op[i] == Instruction::Operation::NOP)return i;
    }
    return -1;
}

long long Tomasulo::ArithmeticComponentStatus::getFreeAddComponent() const {
    for (int i = Tomasulo::ArithmeticComponentStatus::ADD_BEGIN;
         i < Tomasulo::ArithmeticComponentStatus::ADD_BEGIN + NUMBER_OF_ADD_COMPONENT; ++i) {
        if (op[i] == Instruction::Operation::NOP)return i;
    }
    return -1;
}
