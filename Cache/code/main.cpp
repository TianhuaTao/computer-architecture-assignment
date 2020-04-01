#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
struct Address
{
    size_t addr;
    Address(size_t addr):addr(addr){}
};

enum Associative{
    full, direct, way4, way8
};

enum ReplacementPolicy{
    lru, randomReplace, binaryTree
};

enum WritePolicy{
    allocate_through,allocate_back, noalloc_through,noalloc_back
};
struct Option
{
    int blockSize;
    bool allocate;
    bool writeBack;
    Associative associative;
    ReplacementPolicy replacement;
};
struct Op{
    bool read;
    Address address;
    Op(bool read, Address addr):read(read),address(addr){}
};

int main(int argc, char* argv[]){
    const char * filename = argv[1];
    std::ifstream ifs (filename);
    std::vector<Op> ops;
    while (ifs)
    {
        std::string rw;
        size_t addr;
        ifs >> rw >> std::hex >> addr;
        bool read;
        if(rw == "w" || rw == "s")read = false;
        else read = true;
        ops.emplace_back(read, addr);
    }
    
    return 0;
}