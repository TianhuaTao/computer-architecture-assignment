#include "Instruction.h"
#include "Tomasulo.h"
#include <iostream>
#include <queue>
#include <regex>
using namespace std;

void usage() {
    cout << "Usage: Tomasulo [-v] file ..." << endl;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        usage();
        return 0;
    }
    bool verbose = false;
    int i;
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] != '-')
            break;
        if (string(argv[i]) == "-v")
            verbose = true;
    }
    for (; i < argc; ++i) {
        string filename = argv[i];
        string outfile;
        outfile = regex_replace(filename, regex("nel$"), "log");    // *.nel to *.log
        auto instructionQueue = Instruction::getInstructionQueue(filename);
        Tomasulo t(instructionQueue);
        t.setVerbose(verbose);
        t.setLogFilename(outfile);
        t.run();
        t.writeLog();
    }

    return 0;
}
