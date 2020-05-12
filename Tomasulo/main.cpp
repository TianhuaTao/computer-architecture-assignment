#include <iostream>
#include <queue>
#include "Instruction.h"
#include "Tomasulo.h"
using namespace std;

void usage() {
    cout << "Usage: Tomasulo file ..." << endl;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        usage();
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        string filename = argv[i];
        string outfile = filename+".log";
        auto instructionQueue = Instruction::getInstructionQueue(filename);
        Tomasulo t(instructionQueue);
        t.setLogFilename(outfile);
        t.run();
    }

    return 0;
}
