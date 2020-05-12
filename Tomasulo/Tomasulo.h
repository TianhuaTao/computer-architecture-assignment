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
    Tomasulo(const std::vector<Instruction>& queue):instructionQueue (queue),logger(*this, "a.log",queue) {
        reservationStation.clear();
        registerStatus.clear();
        loadBuffer.clear();
        arithmeticComponentStatus.clear();
    };
    ~Tomasulo(){};
    void setInstructionQueue(const std::vector<Instruction>& queue){
        instructionQueue = queue;
    }
    void run(bool step = false);
    void setLogFilename(const std::string& name){logger.setOutFilename(name);}

    static const size_t MASK_32_BIT = ~((~0ul)<<32ul);
private:
    struct LoadBuffer{
        bool busy[NUMBER_OF_LOAD_BUFFER];
        long long value[NUMBER_OF_LOAD_BUFFER];

        bool executing[NUMBER_OF_LOAD_BUFFER];
        size_t pc[NUMBER_OF_LOAD_BUFFER];

        bool empty() const{
            for (bool i : busy) {
                if(i)return false;
            }
            return true;
        };
        void clear(){
            memset(busy,0, sizeof(busy));
            memset(value,0, sizeof(value));

            memset(executing,0, sizeof(executing));
        }

        /// return the index of any free buffer,
        /// if all buffers are busy
        /// return -1
        long long getFreeBuffer() const{
            for (int i = 0; i < NUMBER_OF_LOAD_BUFFER; ++i) {
                if(!busy[i])return i;
            }
            return -1;
        }

        void use(long long bufferId,long long v ,size_t ins_pc){
            assert(!busy[bufferId]);
            busy[bufferId] = true;
            value[bufferId]= v;
            pc[bufferId]=ins_pc;
        }

        static unsigned long long getHashTag(long long index) { return LOAD_BUFFER_TAG| static_cast<unsigned  long long >(index);}
    };


    struct RegisterStatus{
        static constexpr long long MAX_REGISTER_ALLOWED = 32;
        long long value[MAX_REGISTER_ALLOWED];
        bool tag[MAX_REGISTER_ALLOWED];
        bool empty() const;
        void clear();
        void receiveBroadcast(long long target, long long value);
        void setTag(long long registerId, long long waitingTag){
            tag[registerId] = true;
            value[registerId]=waitingTag;
        }
    };
    struct ReservationStation{
        static constexpr long long ADD_BEGIN = 0;
        static constexpr long long MULT_BEGIN = NUMBER_OF_ADD_RESERVATION;
        static constexpr long long TOTAL_STATION_COUNT = NUMBER_OF_ADD_RESERVATION+NUMBER_OF_MULTIPLY_RESERVATION;

        bool busy[TOTAL_STATION_COUNT];
        Instruction::Operation op[TOTAL_STATION_COUNT];
        long long tagSource0[TOTAL_STATION_COUNT];
        long long tagSource1[TOTAL_STATION_COUNT];
        long long source0[TOTAL_STATION_COUNT];
        long long source1[TOTAL_STATION_COUNT];
        bool executing[TOTAL_STATION_COUNT];
        size_t pc[TOTAL_STATION_COUNT];

        bool empty() const;
        void clear();
        long long getFreeStation(Instruction::Operation next) const ;
        void use(long long id, const Instruction &ins,RegisterStatus &registerStatus, size_t ins_pc);
        void receiveBroadcast(long long target, long long value);
        static unsigned long long getHashTag(long long index) { return RESERVATION_STATION_TAG| static_cast<unsigned long long >(index);}
    };
    struct ArithmeticComponentStatus{
        static constexpr long long ADD_BEGIN = 0;
        static constexpr long long MULT_BEGIN = ADD_BEGIN+NUMBER_OF_ADD_COMPONENT;
        static constexpr long long LOAD_BEGIN = MULT_BEGIN+NUMBER_OF_MULTIPLY_COMPONENT;
        static constexpr long long TOTAL_COMPONENT_COUNT = NUMBER_OF_ADD_COMPONENT+NUMBER_OF_MULTIPLY_COMPONENT+NUMBER_OF_LOAD_COMPONENT;
        long long tag[TOTAL_COMPONENT_COUNT];
        Instruction::Operation op[TOTAL_COMPONENT_COUNT];
        long long cyclesRemain[TOTAL_COMPONENT_COUNT];
        long long startCycle[TOTAL_COMPONENT_COUNT];
        long long RS[TOTAL_COMPONENT_COUNT];
        long long input0[TOTAL_COMPONENT_COUNT];
        long long input1[TOTAL_COMPONENT_COUNT];
        size_t pc[TOTAL_COMPONENT_COUNT];
        bool empty() const;
        void clear(){
            for (int i = 0; i < TOTAL_COMPONENT_COUNT; ++i) {
                op[i] = Instruction::Operation::NOP;
                tag[i] = 0;
                cyclesRemain[i]=0;
                startCycle[i] = 0;
                RS[i] = 0;
            }
        }
        void use(long long ComponentId,long long hashTag, Instruction::Operation operation, long long v1, long long v2, size_t ins_pc){
            op[ComponentId]=operation;
            RS[ComponentId]=hashTag;
            pc[ComponentId]=ins_pc;
            if(operation==Instruction::Operation::ADD||operation==Instruction::Operation::SUB){ // Add
                cyclesRemain[ComponentId]=3;
            } else if (operation==Instruction::Operation::MUL){
                cyclesRemain[ComponentId]=4;
            } else if (operation==Instruction::Operation::DIV){
                cyclesRemain[ComponentId]=(v2==0?1:4);
            }else if (operation==Instruction::Operation::LD){
                cyclesRemain[ComponentId]=3;
            }else if (operation==Instruction::Operation::JUMP){
                cyclesRemain[ComponentId]=1;
            } else{
                assert(false);
            }
            input0[ComponentId]=v1;input1[ComponentId]=v2;
        }
        long long getFreeAddComponent()const ;
        long long getFreeMultComponent()const;
        long long getFreeLoadComponent()const;
    };

private:
    std::vector<Instruction> instructionQueue;
    LoadBuffer loadBuffer;
    RegisterStatus registerStatus;
    ReservationStation reservationStation;
    ArithmeticComponentStatus arithmeticComponentStatus;
    size_t cycleCount = 1;
    bool jumpIssued= false;
    long long jumpDiff = 0;
    Logger logger;
    size_t pc=0;
public:
    size_t getCycleCount() const;

private:
    static const size_t MASK_LOW_BITS = 16u;

    static const unsigned long long REGISTER_STATUS_TAG = 1u<<MASK_LOW_BITS;
    static const unsigned long long ARITHMETIC_COMPONENT_STATUS_TAG=REGISTER_STATUS_TAG<<1u;
    static const unsigned long long RESERVATION_STATION_TAG = ARITHMETIC_COMPONENT_STATUS_TAG<<1u;
    static const unsigned long long LOAD_BUFFER_TAG = RESERVATION_STATION_TAG<<1u;

    // helper function
    static const size_t MASK_HIGH = (~0ul)<<MASK_LOW_BITS;   // FFFF FFFF FFFF 0000
    static const size_t MASK_LOW = ~MASK_HIGH;   // FFFF FFFF FFFF 0000
    // 0000 0000 FFFF FFFF
};


#endif //TOMASULO_TOMASULO_H
