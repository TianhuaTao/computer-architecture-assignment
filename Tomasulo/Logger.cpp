//
// Created by Sam on 2020/5/11.
//

#include "Logger.h"
#include "Tomasulo.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace std;

template <typename T>
std::string int_to_hex(T i) {
    std::stringstream stream;
    stream << "0x"
           << std::setfill('0') << std::setw(sizeof(T) * 2)
           << std::hex << i;
    return stream.str();
}

void horizontalLine(int n) {
    string s(n, '-');
    cout << s << endl;
}

std::string toString(Instruction::Operation op) {
    switch (op) {
    case Instruction::Operation::NOP:
        return "NOP";
    case Instruction::Operation::ADD:
        return "ADD";
    case Instruction::Operation::SUB:
        return "SUB";
    case Instruction::Operation::MUL:
        return "MUL";
    case Instruction::Operation::DIV:
        return "DIV";
    case Instruction::Operation::LD:
        return "LD";
    case Instruction::Operation::JUMP:
        return "JUMP";
    default:
        return "";
    }
}
std::string toString(Instruction::Operand o) {
    if (o.isRegister()) {
        return "R" + to_string(o.getValue());
    } else {
        return int_to_hex(o.getValue());
    }
}
std::string toString(Instruction ins) {
    std::stringstream stream;
    stream << std::left << std::setfill(' ') << std::setw(40);
    if (ins.operation == Instruction::Operation::LD)
        stream << toString(ins.operation) + ',' + toString(ins.operands[0]) + ',' + toString(ins.operands[1]);
    else
        stream << toString(ins.operation) + ',' + toString(ins.operands[0]) + ',' + toString(ins.operands[1]) + ',' + toString(ins.operands[2]);
    return stream.str();
}

std::string Logger::hashTagToLabel(int hashTag) const {
    if (hashTag == 0)
        return string();
    string s;
    size_t high = (size_t)hashTag & Tomasulo::MASK_HIGH;
    size_t low = (size_t)hashTag & Tomasulo::MASK_LOW;
    switch (high) {
    case Tomasulo::LOAD_BUFFER_TAG:
        s = "LB" + to_string(low + 1);
        break;
    case Tomasulo::RESERVATION_STATION_TAG:
        if (low < Tomasulo::ReservationStation::MULT_BEGIN)
            s = "ARS" + to_string(low + 1);
        else
            s = "MRS" + to_string(low + 1 - Tomasulo::ReservationStation::MULT_BEGIN);
        break;
    case Tomasulo::REGISTER_STATUS_TAG:
        s = "R" + to_string(low);
        break;
    default:
        break;
    }

    return s;
}

void Logger::printStatus() const {
    cout << "Cycle " << tomasulo.cycleCount << " (PC=" << tomasulo.pc << ")\n";
    printReservationStation();
    cout << endl;
    printLoadBuffer();
    cout << endl;
    printRegisters();
    cout << endl;
    printArithmeticComponent();
    cout << endl;
}

void Logger::printLoadBuffer() const {
    cout << "Load Buffer\n";
    const int colCnt = 3;
    int cw = 20;
    vector<string> data(3);
    data[1] = "Busy";
    data[2] = "Address";
    horizontalLine(colCnt * cw + colCnt + 1);
    printRow(colCnt, data, cw);
    horizontalLine(colCnt * cw + colCnt + 1);

    for (int i = 0; i < NUMBER_OF_LOAD_BUFFER; ++i) {
        data[0] = "LB " + std::to_string(i + 1);
        data[1] = tomasulo.loadBuffer.busy[i] ? "*" : "";
        if (tomasulo.loadBuffer.busy[i]) {
            data[2] = int_to_hex(tomasulo.loadBuffer.value[i]);
        } else {
            data[2] = "";
        }
        printRow(colCnt, data, cw);
    }
    horizontalLine(colCnt * cw + colCnt + 1);
}

void Logger::printRow(int columnCount, const std::vector<std::string> &data, int cellWidth) const {
    cout << "|";
    for (int i = 0; i < columnCount; ++i) {
        int width;
        width = cellWidth < data[i].size() ? (int)data[i].size() : cellWidth;
        int spaceCount = width - (int)data[i].size();
        int space_ahead = spaceCount / 2;
        int space_after = spaceCount - space_ahead;
        for (int j = 0; j < space_ahead; ++j) {
            cout << ' ';
        }
        cout << data[i];
        for (int j = 0; j < space_after; ++j) {
            cout << ' ';
        }
        cout << "|";
    }
    cout << endl;
}

void Logger::printReservationStation() const {
    cout << "Reservation Station\n";
    const int colCnt = 7;
    vector<string> data = {"", "Busy", "Op", "Vj", "Vk", "Qj", "Qk"};
    horizontalLine(colCnt * 12 + colCnt + 1);
    printRow(colCnt, data, 12);
    horizontalLine(colCnt * 12 + colCnt + 1);

    for (int i = 0; i < Tomasulo::ReservationStation::TOTAL_STATION_COUNT; ++i) {
        if (i < Tomasulo::ReservationStation::MULT_BEGIN) {
            data[0] = "Ars " + std::to_string(i + 1);
        } else {
            data[0] = "Mrs " + std::to_string(i + 1 - Tomasulo::ReservationStation::MULT_BEGIN);
        }
        auto &rs = tomasulo.reservationStation;
        if (rs.busy[i]) {
            data[1] = "*";
            data[2] = toString(rs.op[i]);
            if (rs.tagSource0[i] == 0) {
                data[3] = int_to_hex(rs.source0[i]);
                data[5] = "";
            } else {
                data[3] = "";
                data[5] = hashTagToLabel(rs.tagSource0[i]);
            }
            if (rs.tagSource1[i] == 0) {
                data[4] = int_to_hex(rs.source1[i]);
                data[6] = "";
            } else {
                data[4] = "";
                data[6] = hashTagToLabel(rs.tagSource1[i]);
            }

        } else {
            for (int j = 1; j < 7; ++j) {
                data[j] = "";
            }
        }
        printRow(colCnt, data, 12);
    }
    horizontalLine(colCnt * 12 + colCnt + 1);
}

Logger::Logger(Tomasulo &t, const std::string &outFilename, const std::vector<Instruction> &queue)
    : tomasulo(t),
      outFilename(outFilename),
      instructionQueue(queue),
      records(queue.size()) {
}

void Logger::setOutFilename(const string &outFilename) {
    this->outFilename = outFilename;
}

void Logger::printRegisters() const {
    cout << "Registers\n";
    const int colCnt = 3;
    const int cw = 20;
    vector<string> data = {"", "State", "Value"};
    horizontalLine(colCnt * cw + colCnt + 1);
    printRow(colCnt, data, cw);
    horizontalLine(colCnt * cw + colCnt + 1);
    for (int i = 0; i < Tomasulo::RegisterStatus::MAX_REGISTER_ALLOWED; ++i) { // print 8
        data[0] = "R" + to_string(i);
        if (tomasulo.registerStatus.tag[i]) {
            data[1] = hashTagToLabel(tomasulo.registerStatus.value[i]);
            data[2] = "";
        } else {
            data[1] = "";
            data[2] = int_to_hex(tomasulo.registerStatus.value[i]);
        }

        printRow(colCnt, data, cw);
    }
    horizontalLine(colCnt * cw + colCnt + 1);
}

void Logger::printArithmeticComponent() const {
    cout << "Arithmetic Component\n";
    const int colCnt = 4;
    const int cw = 20;
    vector<string> data = {"", "Current", "RS", "Cycles remain"};
    horizontalLine(colCnt * cw + colCnt + 1);
    printRow(colCnt, data, cw);
    horizontalLine(colCnt * cw + colCnt + 1);

    for (int i = 0; i < Tomasulo::ArithmeticComponentStatus::TOTAL_COMPONENT_COUNT; ++i) {
        if (i < Tomasulo::ArithmeticComponentStatus::MULT_BEGIN) {
            data[0] = "Add " + std::to_string(i + 1);
        } else if (i < Tomasulo::ArithmeticComponentStatus::LOAD_BEGIN) {
            data[0] = "Mult " + std::to_string(i + 1 - Tomasulo::ArithmeticComponentStatus::MULT_BEGIN);
        } else {
            data[0] = "Load " + std::to_string(i + 1 - Tomasulo::ArithmeticComponentStatus::LOAD_BEGIN);
        }
        auto &ac = tomasulo.arithmeticComponentStatus;
        if (ac.op[i] != Instruction::Operation::NOP) {
            data[1] = toString(ac.op[i]);
            data[2] = hashTagToLabel(ac.RS[i]);
            data[3] = to_string(ac.cyclesRemain[i]);
        } else {
            for (int j = 1; j < colCnt; ++j) {
                data[j] = "";
            }
        }
        printRow(colCnt, data, cw);
    }
    horizontalLine(colCnt * cw + colCnt + 1);
}

void Logger::printSummary() const {
    cout << "Summary\n";
    const int colCnt = 5;
    const int cw = 14;
    vector<string> data = {"PC Number", "Instruction", "Issue", "Exec Comp", "Write Result"};
    data[1].resize(40, ' ');
    horizontalLine(colCnt * cw + colCnt + 1 - cw + 40);
    printRow(colCnt, data, cw);
    horizontalLine(colCnt * cw + colCnt + 1 - cw + 40);
    for (int i = 0; i < instructionQueue.size(); ++i) {
        data[0] = to_string(i);
        data[1] = toString(instructionQueue[i]);
        data[2] = to_string(records[i].issue);
        data[3] = to_string(records[i].execCemplete);
        data[4] = to_string(records[i].writeResult);
        printRow(colCnt, data, cw);
    }
    horizontalLine(colCnt * cw + colCnt + 1 - cw + 40);
}

void Logger::logIssue(int lineno) {
    if (records[lineno].issue == 0) // record the first time
        records[lineno].issue = tomasulo.getCycleCount();
}

void Logger::logReady(int lineno) {
    if (records[lineno].ready == 0) // record the first time
        records[lineno].ready = tomasulo.getCycleCount();
}

void Logger::logExecuteComplete(int lineno) {
    if (records[lineno].execCemplete == 0) // record the first time
        records[lineno].execCemplete = tomasulo.getCycleCount();
}

void Logger::logWriteResult(int lineno) {
    if (records[lineno].writeResult == 0) // record the first time
        records[lineno].writeResult = tomasulo.getCycleCount();
}

void Logger::writeLog() {
    ofstream os(outFilename);
    for (int i = 0; i < records.size(); ++i) {
        os << records[i].issue << " " << records[i].execCemplete << " " << records[i].writeResult << "\n";
    }
    os.close();
}
