#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include "Cache.hpp"

std::string filenames[] = {
//        "test.trace",
        "astar.trace",
        "bodytrack_1m.trace",
        "bzip2.trace",
        "canneal.uniq.trace",
        "gcc.trace",
        "mcf.trace",
        "perlbench.trace",
        "streamcluster.trace",
        "swim.trace",
        "twolf.trace"
};
bool has_rw[] = {
//        true,
        true,
        false,
        true,
        false,
        true,
        true,
        true,
        false,
        true,
        true
};
Option options[] = {
        {8,  true,  true,  direct, lru},
        {32, true,  true,  direct, lru},
        {64, true,  true,  direct, lru},
        {8,  true,  true,  full,   lru},
        {32, true,  true,  full,   lru},
        {64, true,  true,  full,   lru},
        {8,  true,  true,  way4,   lru},
        {32, true,  true,  way4,   lru},
        {64, true,  true,  way4,   lru},
        {8,  true,  true,  way8,   lru},
        {32, true,  true,  way8,   lru},
        {64, true,  true,  way8,   lru},

//        { 8,true,  true,way8, lru},
        {8,  true,  true,  way8,   randomReplace},
        {8,  true,  true,  way8,   binaryTree},

//        { 8, true,  true,way8, lru},
        {8,  false, true,  way8,   lru},
        {8,  true,  false, way8,   lru},
        {8,  false, false, way8,   lru},
};

int main(int argc, char *argv[]) {
    int id = 0;
    for (auto &filename : filenames) {
        std::cout << "******************  " << filename << "  ******************" << std::endl;
        std::ifstream ifs("./data/trace/" + filename);
        std::vector<Op> ops;
        while (ifs) {
            if (has_rw[id]) {
                std::string rw;
                size_t addr;
                ifs >> rw >> std::hex >> addr;
                if (rw.empty()) continue;
                bool read;
                if (rw == "w" || rw == "s")read = false;
                else read = true;
                ops.emplace_back(read, Address(addr));
            } else {
                size_t addr;
                ifs >> std::hex >> addr;
                ops.emplace_back(true, Address(addr));
            }
        }
        for (Option &opt: options) {
            Cache cache(ops, opt);
            cache.run();
            cache.report();
            std::cout << std::endl;
        }
        id++;
        std::cout << "********************" << "********************" << "********************\n" << std::endl;
    }
    return 0;
}