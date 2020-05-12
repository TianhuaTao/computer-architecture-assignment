//
// Created by Sam on 2020/5/11.
//

#ifndef TOMASULO_LOGGER_H
#define TOMASULO_LOGGER_H

#include <string>
#include <queue>
#include <vector>
#include "Instruction.h"
class Tomasulo;
class Instruction;
class Logger {
public:
    Logger(Tomasulo& t, const std::string& outFilename, const std::vector<Instruction>& queue);

    void printSummary()const ;

    void printStatus()const;

    void logIssue(int lineno);
    void logReady(int lineno);
    void logExecuteComplete(int lineno);
    void logWriteResult(int lineno);

    struct Record{size_t issue=0; size_t execCemplete=0;size_t writeResult=0;size_t ready = 0;};
private:
    Tomasulo &tomasulo;
    std::vector<Record> records;
private:
    void printReservationStation()const;
    void printLoadBuffer()const;
    void printRegisters()const;
    void printArithmeticComponent()const;

    void printRow(int columnCount,  const std::vector<std::string> &data ,int cellWidth=0)const;

    std::string hashTagToLabel(long long hashTag)const ;

    std::string outFilename;
public:
    void setOutFilename(const std::string &outFilename);

private:
    std::vector<Instruction> instructionQueue;
};


#endif //TOMASULO_LOGGER_H
