//
// Created by Sam on 2020/5/11.
//

#ifndef TOMASULO_TOMASULO_H
#define TOMASULO_TOMASULO_H

#include "Instruction.h"
#include "Logger.h"
#include <cstring>
const int NUMBER_OF_ADD_COMPONENT = 3;
const int NUMBER_OF_MULTIPLY_COMPONENT = 2;
const int NUMBER_OF_LOAD_COMPONENT = 2;

const int NUMBER_OF_ADD_RESERVATION = 6;
const int NUMBER_OF_MULTIPLY_RESERVATION = 3;
const int NUMBER_OF_LOAD_BUFFER = 3;

const int LOAD_CYCLES = 3;
const int JUMP_CYCLES = 1;
const int ADD_CYCLES = 3;
const int SUB_CYCLES = 3;
const int MUL_CYCLES = 4;
const int DIV_CYCLES = 3;
const int DIV_ZERO_CYCLES = 1;

class Tomasulo {
    friend class Logger;

public:
    Tomasulo(const std::vector<Instruction> &queue) : instructionQueue(queue), logger(*this, "a.log", queue) {
        reservationStation.clear();
        registerStatus.clear();
        loadBuffer.clear();
        arithmeticComponentStatus.clear();
    };
    ~Tomasulo(){};
    void setInstructionQueue(const std::vector<Instruction> &queue) {
        instructionQueue = queue;
    }
    void run(bool step = false);
    void setLogFilename(const std::string &name) { logger.setOutFilename(name); }
    void writeLog() { logger.writeLog(); }
    //    static const unsigned int MASK_32_BIT = ~((~0ul)<<32ul);
private:
    struct LoadBuffer {
        bool busy[NUMBER_OF_LOAD_BUFFER];
        int value[NUMBER_OF_LOAD_BUFFER];
        bool executing[NUMBER_OF_LOAD_BUFFER];      // 是否已经开始执行了
        int pc[NUMBER_OF_LOAD_BUFFER];
        int issueCycle[NUMBER_OF_LOAD_BUFFER];
        int readyTime[NUMBER_OF_LOAD_BUFFER];

        bool empty() const {
            for (bool i : busy) {
                if (i)
                    return false;
            }
            return true;
        };
        void clear() {
            memset(busy, 0, sizeof(busy));
            memset(value, 0, sizeof(value));
            memset(executing, 0, sizeof(executing));
        }

        /// return the index of any free buffer,
        /// if all buffers are busy
        /// return -1
        int getFreeBuffer() const {
            for (int i = 0; i < NUMBER_OF_LOAD_BUFFER; ++i) {
                if (!busy[i])
                    return i;
            }
            return -1;
        }

        void use(int bufferId, int v, int ins_pc, int currentCycle) {
            assert(!busy[bufferId]);
            busy[bufferId] = true;
            value[bufferId] = v;
            pc[bufferId] = ins_pc;
            executing[bufferId] = false;
            issueCycle[bufferId] = currentCycle;
            readyTime[bufferId] = 0;
        }

        static int getHashTag(int index) { return (int)(LOAD_BUFFER_TAG | static_cast<unsigned int>(index)); }
    };

    struct RegisterStatus {
        static constexpr int MAX_REGISTER_ALLOWED = 32;
        int value[MAX_REGISTER_ALLOWED];
        bool tag[MAX_REGISTER_ALLOWED];
        bool empty() const;
        void clear();
        void receiveBroadcast(int target, int value);
        void setTag(int registerId, int waitingTag) {
            tag[registerId] = true;
            value[registerId] = waitingTag;
        }
    };
    struct ReservationStation {
        static constexpr int ADD_BEGIN = 0;
        static constexpr int MULT_BEGIN = NUMBER_OF_ADD_RESERVATION;
        static constexpr int TOTAL_STATION_COUNT = NUMBER_OF_ADD_RESERVATION + NUMBER_OF_MULTIPLY_RESERVATION;

        bool busy[TOTAL_STATION_COUNT];
        Instruction::Operation op[TOTAL_STATION_COUNT];
        int tagSource0[TOTAL_STATION_COUNT];        // Qj
        int tagSource1[TOTAL_STATION_COUNT];        // Qk
        int source0[TOTAL_STATION_COUNT];           // Vj
        int source1[TOTAL_STATION_COUNT];           // Vk
        bool executing[TOTAL_STATION_COUNT];
        int pc[TOTAL_STATION_COUNT];
        int issueCycle[TOTAL_STATION_COUNT];
        int readyTime[TOTAL_STATION_COUNT];

        bool empty() const;
        void clear();
        int getFreeStation(Instruction::Operation next) const;
        void use(int id, const Instruction &ins, RegisterStatus &registerStatus, int ins_pc, int currentCycle);
        void receiveBroadcast(int target, int value);
        static int getHashTag(int index) { return (int)(RESERVATION_STATION_TAG | static_cast<unsigned int>(index)); }
    };
    struct ArithmeticComponentStatus {
        static constexpr int ADD_BEGIN = 0;
        static constexpr int MULT_BEGIN = ADD_BEGIN + NUMBER_OF_ADD_COMPONENT;
        static constexpr int LOAD_BEGIN = MULT_BEGIN + NUMBER_OF_MULTIPLY_COMPONENT;
        static constexpr int TOTAL_COMPONENT_COUNT = NUMBER_OF_ADD_COMPONENT + NUMBER_OF_MULTIPLY_COMPONENT + NUMBER_OF_LOAD_COMPONENT;
        int tag[TOTAL_COMPONENT_COUNT];                         // 对外广播时的 tag
        Instruction::Operation op[TOTAL_COMPONENT_COUNT];       // 对应原始指令
        int cyclesRemain[TOTAL_COMPONENT_COUNT];    // 剩余的周期
        int startCycle[TOTAL_COMPONENT_COUNT];      // 开始执行的 cycle，用来记录log，没用使用到
        int RS[TOTAL_COMPONENT_COUNT];              // 该指令来自的 RS
        int input0[TOTAL_COMPONENT_COUNT];          // 第一个操作数
        int input1[TOTAL_COMPONENT_COUNT];          // 第二个操作数
        int pc[TOTAL_COMPONENT_COUNT];              // 该指令在指令序列的序号
        bool empty() const;
        void clear() {
            for (int i = 0; i < TOTAL_COMPONENT_COUNT; ++i) {
                op[i] = Instruction::Operation::NOP;
                tag[i] = 0;
                cyclesRemain[i] = 0;
                startCycle[i] = 0;
                RS[i] = 0;
            }
        }
        void use(int ComponentId, int hashTag, Instruction::Operation operation, int v1, int v2, int ins_pc) {
            op[ComponentId] = operation;
            RS[ComponentId] = hashTag;
            pc[ComponentId] = ins_pc;
            if (operation == Instruction::Operation::ADD || operation == Instruction::Operation::SUB) { // Add
                cyclesRemain[ComponentId] = 3;
            } else if (operation == Instruction::Operation::MUL) {
                cyclesRemain[ComponentId] = 4;
            } else if (operation == Instruction::Operation::DIV) {
                cyclesRemain[ComponentId] = (v2 == 0 ? 1 : 4);
            } else if (operation == Instruction::Operation::LD) {
                cyclesRemain[ComponentId] = 3;
            } else if (operation == Instruction::Operation::JUMP) {
                cyclesRemain[ComponentId] = 1;
            } else {
                assert(false);
            }
            input0[ComponentId] = v1;
            input1[ComponentId] = v2;
        }
        int getFreeAddComponent() const;
        int getFreeMultComponent() const;
        int getFreeLoadComponent() const;
    };

private:
    std::vector<Instruction> instructionQueue;
    LoadBuffer loadBuffer;
    RegisterStatus registerStatus;
    ReservationStation reservationStation;
    ArithmeticComponentStatus arithmeticComponentStatus;
    int cycleCount = 1;
    bool jumpIssued = false;
    int jumpDiff = 0;
    Logger logger;
    int pc = 0;
    bool verbose = false;

public:
    void setVerbose(bool v) {
        this->verbose = v;
    }
    int getCycleCount() const;

private:
    static const unsigned int MASK_LOW_BITS = 16u;

    static const unsigned int REGISTER_STATUS_TAG = 1u << MASK_LOW_BITS;
    static const unsigned int ARITHMETIC_COMPONENT_STATUS_TAG = REGISTER_STATUS_TAG << 1u;
    static const unsigned int RESERVATION_STATION_TAG = ARITHMETIC_COMPONENT_STATUS_TAG << 1u;
    static const unsigned int LOAD_BUFFER_TAG = RESERVATION_STATION_TAG << 1u;

    // helper function
    static const unsigned int MASK_HIGH = (~0u) << MASK_LOW_BITS; // FFFF 0000
    static const unsigned int MASK_LOW = ~MASK_HIGH;              // FFFF 0000

};

#endif //TOMASULO_TOMASULO_H
