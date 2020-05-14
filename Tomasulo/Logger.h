//
// Created by Sam on 2020/5/11.
//

#ifndef TOMASULO_LOGGER_H
#define TOMASULO_LOGGER_H

#include "Instruction.h"
#include <queue>
#include <string>
#include <vector>

class Tomasulo;
class Instruction;

/**
 * @brief 记录日志的封装类
 * 
 */
class Logger {
public:
    Logger(Tomasulo &t, const std::string &outFilename, const std::vector<Instruction> &queue);

    void printSummary() const;
    void printStatus() const;

    /**
     * @brief 
     * 
     * @param lineno 指令的行号
     */
    void logIssue(int lineno);
    void logReady(int lineno);
    void logExecuteComplete(int lineno);
    void logWriteResult(int lineno);
    void writeLog();
    struct Record {
        size_t issue = 0;
        size_t execCemplete = 0;
        size_t writeResult = 0;
        size_t ready = 0;
    };

private:
    Tomasulo &tomasulo;
    std::vector<Record> records;        // real data

private:
    void printReservationStation() const;
    void printLoadBuffer() const;
    void printRegisters() const;
    void printArithmeticComponent() const;
    void printRow(int columnCount, const std::vector<std::string> &data, int cellWidth = 0) const;
    std::string hashTagToLabel(int hashTag) const;
    std::string outFilename;

public:
    void setOutFilename(const std::string &outFilename);

private:
    std::vector<Instruction> instructionQueue;
};

#endif //TOMASULO_LOGGER_H
