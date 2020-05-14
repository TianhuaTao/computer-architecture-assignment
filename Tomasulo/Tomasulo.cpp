//
// Created by Sam on 2020/5/11.
//

#include "Tomasulo.h"
#include <algorithm>
#include <vector>

/**
 * @brief Tomasulo 的主方法
 * 分为以下阶段：Write Result, Issue, Execute, RS update, Load Buffer update
 * 其中重要的是 Write Result 要最先，这样才可以释放部件，并且其他指令会需要 Write Result 的结果
 * Issue 要在 RS update 和 Load Buffer update之前，
 * 因为有的指令Issue完之后直接就可以进入 RS 和 LB 了
 * 其他的顺序没有太大的关系
 * 
 * 详细内容：
 * Write Result --  取 arithmeticComponentStatus 中完成的指令，如果是JUMP，释放保留站，
 *                  修改pc；如果是其他的，释放保留站，计算结果，并广播
 * Issue --         进入保留站或者是 Load Buffer（上个周期执行完指令的已经从中清除了）
 *                  如果是 JUMP 的话，要设置 jumpIssued，暂停后续发射
 *                  由于 JUMP 会暂停发射了，因此不会有多个 JUMP 都发射了的情况
 * Execute --       递减剩余周期数，并做 log
 * RS update --     取就绪的指令，进入运算部件，操作的时候如下：
 *                  把所有就绪的行序号放进一个 vector，按照 ready time 然后 issue time
 *                  排序，按顺序进入运算部件
 * Load Buffer update --    和 RS 相同
 * @param step 原本是用来单步运行的，实际没用用到
 */
void Tomasulo::run(bool step) {
    while (true) {
        bool finished = instructionQueue.size() == pc &&
                        reservationStation.empty() &&
                        loadBuffer.empty() &&
                        arithmeticComponentStatus.empty();

        if (finished) {
            assert(registerStatus.empty());
            if (verbose)
                logger.printSummary();
            return;
        }

        /* cycle start */

        // Write Result (of last cycle)
        for (int j = 0; j < ArithmeticComponentStatus::TOTAL_COMPONENT_COUNT; ++j) {
            if (arithmeticComponentStatus.op[j] != Instruction::Operation::NOP) { // working
                if (arithmeticComponentStatus.cyclesRemain[j] == 0) {             // complete
                    if (arithmeticComponentStatus.op[j] == Instruction::Operation::JUMP) {
                        int target = arithmeticComponentStatus.RS[j]; // broadcast target
                        // reset reservation station
                        reservationStation.busy[(unsigned int)target & MASK_LOW] = false;
                        // reset the Arithmetic Component
                        arithmeticComponentStatus.op[j] = Instruction::Operation::NOP;
                        jumpIssued = false;
                        logger.logWriteResult(arithmeticComponentStatus.pc[j]);
                        auto s1 = arithmeticComponentStatus.input0[j];
                        auto s2 = arithmeticComponentStatus.input1[j];
                        if (s1 == s2) {
                            pc += jumpDiff;
                            pc--; // JUMP itself
                        }
                    } else {
                        int target = arithmeticComponentStatus.RS[j]; // broadcast target

                        // calculate broadcast value
                        int value = 0;
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
                            loadBuffer.busy[(unsigned int)target & MASK_LOW] = false;
                        } else {
                            reservationStation.busy[(unsigned int)target & MASK_LOW] = false;
                        }

                        // reset the Arithmetic Component
                        arithmeticComponentStatus.op[j] = Instruction::Operation::NOP;

                        // broadcast to RS, Registers
                        reservationStation.receiveBroadcast(target, value);
                        registerStatus.receiveBroadcast(target, value);
                        /* loadBuffer.receiveBroadcast(target, value); */ // loadBuffer doesn't need to receive broadcast

                        logger.logWriteResult(arithmeticComponentStatus.pc[j]);
                    }
                }
            }
        }
        // Issue
        if (pc < instructionQueue.size() && !jumpIssued) {
            Instruction ins = instructionQueue[pc];
            int id;
            switch (ins.operation) {
            case Instruction::Operation::LD:
                id = loadBuffer.getFreeBuffer();
                if (id != -1) {
                    loadBuffer.use(id, ins.operands[1].getValue(), pc, cycleCount); // operands[1] is value
                    registerStatus.setTag(ins.operands[0].getValue(),
                                          Tomasulo::LoadBuffer::getHashTag(id)); // operands[0] is value
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
                    reservationStation.use(id, ins, registerStatus, pc, cycleCount);
                    logger.logIssue(pc);
                    pc++;
                }
                break;
            case Instruction::Operation::JUMP:
                id = reservationStation.getFreeStation(ins.operation);
                if (id != -1) {
                    reservationStation.use(id, ins, registerStatus, pc, cycleCount);
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
            if (arithmeticComponentStatus.op[j] != Instruction::Operation::NOP) { // working
                // no component should be ready, since checked before during Write Result
                assert(arithmeticComponentStatus.cyclesRemain[j] != 0);
                arithmeticComponentStatus.cyclesRemain[j]--;
                if (arithmeticComponentStatus.cyclesRemain[j] == 0) {
                    logger.logExecuteComplete(arithmeticComponentStatus.pc[j]);
                }
            }
        }
        // RS update, if ready, push into ALU
        std::vector<int> ready_rs_id;
        for (int jj = 0; jj < ReservationStation::TOTAL_STATION_COUNT; ++jj) {
            if (reservationStation.busy[jj] && reservationStation.tagSource0[jj] == 0 &&
                reservationStation.tagSource1[jj] == 0 && !reservationStation.executing[jj]) { // ready
                if (reservationStation.readyTime[jj] == 0) {                                   // first time ready
                    reservationStation.readyTime[jj] = cycleCount;
                    logger.logReady(reservationStation.pc[jj]);
                }
                ready_rs_id.push_back(jj);
            }
        }
        // first sort by ready time, then by issue time
        std::sort(ready_rs_id.begin(), ready_rs_id.end(), [&](int lhs, int rhs) {
            if (reservationStation.readyTime[lhs] != reservationStation.readyTime[rhs])
                return reservationStation.readyTime[lhs] < reservationStation.readyTime[rhs];
            else
                return reservationStation.issueCycle[lhs] < reservationStation.issueCycle[rhs];
        });
        for (int id : ready_rs_id) {
            // if there is empty ALU, start executing
            auto operation = reservationStation.op[id];
            int compId = -1;
            if (operation == Instruction::Operation::ADD || operation == Instruction::Operation::SUB ||
                operation == Instruction::Operation::JUMP) {
                compId = arithmeticComponentStatus.getFreeAddComponent();
            } else if (operation == Instruction::Operation::MUL || operation == Instruction::Operation::DIV) {
                compId = arithmeticComponentStatus.getFreeMultComponent();
            }
            if (compId != -1) { // free unit
                arithmeticComponentStatus.use(compId, ReservationStation::getHashTag(id), operation,
                                              reservationStation.source0[id], reservationStation.source1[id],
                                              reservationStation.pc[id]);
                reservationStation.executing[id] = true;
            }
        }

        // Load Buffer update, if there is an empty loader, push into loader
        std::vector<int> readyLB;
        for (int i = 0; i < NUMBER_OF_LOAD_BUFFER; ++i) {
            if (loadBuffer.busy[i] && !loadBuffer.executing[i]) { // is ready
                if (loadBuffer.readyTime[i] == 0) {               // first time ready
                    loadBuffer.readyTime[i] = cycleCount;
                    logger.logReady(loadBuffer.pc[i]);
                }
                readyLB.push_back(i);
            }
        }
        std::sort(readyLB.begin(), readyLB.end(), [&](int lhs, int rhs) {
            if (loadBuffer.readyTime[lhs] != loadBuffer.readyTime[rhs])
                return loadBuffer.readyTime[lhs] < loadBuffer.readyTime[rhs];
            else
                return loadBuffer.issueCycle[lhs] < loadBuffer.issueCycle[rhs];
        });
        for (int buffer_id : readyLB) {
            auto loadComponentId = arithmeticComponentStatus.getFreeLoadComponent();
            if (loadComponentId != -1) {
                if (!readyLB.empty()) {
                    arithmeticComponentStatus.use(loadComponentId, LoadBuffer::getHashTag(buffer_id),
                                                  Instruction::Operation::LD, loadBuffer.value[buffer_id], 0,
                                                  loadBuffer.pc[buffer_id]);
                    loadBuffer.executing[buffer_id] = true;
                }
            }
        }

        if (verbose)
            logger.printStatus();
        cycleCount++;
        /* cycle end */
    }
}

int Tomasulo::getCycleCount() const {
    return cycleCount;
}

bool Tomasulo::ReservationStation::empty() const {
    for (auto i : busy) {
        if (i)
            return false;
    }
    return true;
}

void Tomasulo::ReservationStation::receiveBroadcast(int target, int value) {
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

/**
 * @brief 检查是否有空的 RS
 * 
 * @param next 下一个 Operation 类型
 * @return int 如果有空行，返回行号，否则-1
 */
int Tomasulo::ReservationStation::getFreeStation(Instruction::Operation next) const {
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

/**
 * @brief 进入一个 RS 的函数
 * 
 * @param id 要使用的行序号
 * @param ins 原始指令
 * @param regStatus 寄存器情况
 * @param ins_pc 当前pc
 * @param currentCycle 当前cycle
 */
void Tomasulo::ReservationStation::use(int id, const Instruction &ins, RegisterStatus &regStatus, int ins_pc,
                                       int currentCycle) {
    assert(!busy[id]);
    executing[id] = false; // reset
    issueCycle[id] = currentCycle;
    pc[id] = ins_pc;
    readyTime[id] = 0;

    if (ins.operation == Instruction::Operation::JUMP) {
        assert(ins.operands[0].isImmediate() && ins.operands[1].isRegister() &&
               ins.operands[2].isImmediate());
        op[id] = ins.operation;
        busy[id] = true;

        // src0
        int sourceReg0 = ins.operands[1].getValue();
        if (regStatus.tag[sourceReg0] == 0) { // is immediate value
            source0[id] = regStatus.value[sourceReg0];
            tagSource0[id] = 0;
        } else {
            tagSource0[id] = regStatus.value[sourceReg0];
        }
        // src 1, immediate value
        source1[id] = ins.operands[0].getValue();
        tagSource1[id] = false;

    } else {
        assert(ins.operands[0].isRegister() && ins.operands[1].isRegister() &&
               ins.operands[2].isRegister()); // all registers
        op[id] = ins.operation;
        busy[id] = true;
        int sinkReg, sourceReg0, sourceReg1;
        sinkReg = ins.operands[0].getValue();
        sourceReg0 = ins.operands[1].getValue();
        sourceReg1 = ins.operands[2].getValue();

        // src0
        if (regStatus.tag[sourceReg0] == 0) { // is immediate value
            source0[id] = regStatus.value[sourceReg0];
            tagSource0[id] = 0;
        } else {
            tagSource0[id] = regStatus.value[sourceReg0];
        }

        // src1
        if (regStatus.tag[sourceReg1] == 0) { // is immediate value
            source1[id] = regStatus.value[sourceReg1];
            tagSource1[id] = 0;
        } else {
            tagSource1[id] = regStatus.value[sourceReg1];
        }

        // sink
        regStatus.tag[sinkReg] = true;
        regStatus.value[sinkReg] = getHashTag(id);
    }
}

void Tomasulo::ReservationStation::clear() {
    for (auto &x : busy) {
        x = false;
    }
    for (auto &x : op) {
        x = Instruction::Operation::NOP;
    }
    for (auto &x : tagSource0) {
        x = 0;
    }
    for (auto &x : tagSource1) {
        x = 0;
    }
    for (auto &x : source0) {
        x = 0;
    }
    for (auto &x : source1) {
        x = 0;
    }
    for (auto &x : executing) {
        x = false;
    }
}

void Tomasulo::RegisterStatus::receiveBroadcast(int target, int v) {
    for (int i = 0; i < MAX_REGISTER_ALLOWED; ++i) {
        if (tag[i] && value[i] == target) {
            tag[i] = false;
            value[i] = v;
        }
    }
}

void Tomasulo::RegisterStatus::clear() {
    for (auto &x : tag)
        x = false;
    for (auto &x : value)
        x = 0;
}

bool Tomasulo::RegisterStatus::empty() const {
    for (bool i : tag) {
        if (i)
            return false;
    }
    return true;
}

bool Tomasulo::ArithmeticComponentStatus::empty() const {
    for (auto i : tag) {
        if (i != 0)
            return false;
    }
    return true;
}

int Tomasulo::ArithmeticComponentStatus::getFreeLoadComponent() const {
    for (int i = Tomasulo::ArithmeticComponentStatus::LOAD_BEGIN;
         i < Tomasulo::ArithmeticComponentStatus::LOAD_BEGIN + NUMBER_OF_LOAD_COMPONENT; ++i) {
        if (op[i] == Instruction::Operation::NOP)
            return i;
    }
    return -1;
}

int Tomasulo::ArithmeticComponentStatus::getFreeMultComponent() const {
    for (int i = Tomasulo::ArithmeticComponentStatus::MULT_BEGIN;
         i < Tomasulo::ArithmeticComponentStatus::MULT_BEGIN + NUMBER_OF_MULTIPLY_COMPONENT; ++i) {
        if (op[i] == Instruction::Operation::NOP)
            return i;
    }
    return -1;
}

int Tomasulo::ArithmeticComponentStatus::getFreeAddComponent() const {
    for (int i = Tomasulo::ArithmeticComponentStatus::ADD_BEGIN;
         i < Tomasulo::ArithmeticComponentStatus::ADD_BEGIN + NUMBER_OF_ADD_COMPONENT; ++i) {
        if (op[i] == Instruction::Operation::NOP)
            return i;
    }
    return -1;
}
