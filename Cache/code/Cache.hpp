#if !defined(CACHE_HPP)
#define CACHE_HPP

#include <iostream>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <cassert>

// wrapper class for an address
struct Address {
    size_t addr;

    explicit Address(size_t addr) : addr(addr) {}
};

enum Associative {
    full,
    direct,
    way4,
    way8
};

enum ReplacementPolicy {
    lru,
    randomReplace,
    binaryTree
};

struct Option {
    // general config
    int blockSize;
    bool allocate;
    bool writeBack;
    Associative associative;
    ReplacementPolicy replacement;

    Option(int blockSize, bool allocate,
           bool writeBack,
           Associative associative,
           ReplacementPolicy replacement) : blockSize(blockSize), allocate(allocate), writeBack(writeBack),
                                            associative(associative), replacement(replacement) {}

    // filled and used by Cache
    size_t block_count;
    size_t hit_count = 0;
    size_t miss_count = 0;
    size_t write_hit_count = 0;
    size_t write_miss_count = 0;
    size_t read_hit_count = 0;
    size_t read_miss_count = 0;
    size_t group_count = 0;
    size_t offset_bit_count = 0;
    size_t index_bit_count = 0;
    size_t tag_bit_count = 0;
    size_t dirty_bit_count = 0;
    size_t valid_bit_count = 1;
    size_t offset_mask = 0;
    size_t index_mask = 0;
    size_t tag_mask = 0;
    size_t lru_frame_bits_count = 0;
//    size_t lru_tag_mask = 0;
};

// Operation
struct Op {
    bool read;
    Address address;

    Op(bool read, Address addr) : read(read), address(addr) {}
};

// return floor(log2(n))
// if n == 0, return 0
inline int log2_(size_t n) {
    if (n == 0)
        return 0;
    int p = 0;
    while (n > 0) {
        n = n >> 1u;
        p++;
    }
    return p - 1;
}

struct lruStack {
    char *data_ = nullptr;
    int bytes;
    int bits_;
    size_t stackFrameMask = 0;
    Option *p_option;

    lruStack() {};

    void init(Option &opt);

    ~lruStack() {
        delete[] data_;
    }

    size_t getTag(int i);

    void setTag(int t, size_t tag);

    void makeTop(size_t local_block_id, bool hit);

    size_t bottom() {
        int blocks_in_group = p_option->block_count / p_option->group_count;
        return getTag(blocks_in_group - 1);   // get last
    }
};

struct BinaryTree {
    enum bt_mask {
        MASK0 = 1u << 0u,
        MASK1 = 1u << 1u,
        MASK2 = 1u << 2u,
        MASK3 = 1u << 3u,
        MASK4 = 1u << 4u,
        MASK5 = 1u << 5u,
        MASK6 = 1u << 6u,
        MASK7 = 1u << 7u,
    };
    char *data_ = nullptr;  // at most one char is used, but keep it an array for future use
    //    int bytes;    // since only 4-way and 8-way is used, bytes is always 1, so this is not used
    int bits_;      // bits used
    Option *p_option;

    BinaryTree() {}

    ~BinaryTree() {
        delete[] data_;
    }

    void init(Option &opt);

    void access(size_t local_block_id);

    size_t get_evict() {
        bool b0 = data_[0] & MASK0;
        bool b1 = data_[0] & MASK1;
        bool b2 = data_[0] & MASK2;
        bool b3 = data_[0] & MASK3;
        bool b4 = data_[0] & MASK4;
        bool b5 = data_[0] & MASK5;
        bool b6 = data_[0] & MASK6;
        bool b7 = data_[0] & MASK7;
        if (!b0 && !b1 && !b3)return 0;
        if (!b0 && !b1 && b3)return 1;
        if (!b0 && b1 && !b4)return 2;
        if (!b0 && b1 && b4)return 3;
        if (b0 && !b2 && !b5)return 4;
        if (b0 && !b2 && b5)return 5;
        if (b0 && b2 && !b6)return 6;
        if (b0 && b2 && b6)return 7;
        return 0;
    }
};

struct CacheLineMeta {
    // if hasDirty_ is true, the lower two bits are (dirty, valid), the rest is tag
    // if hasDirty_ is false, the lowest bit is (valid), the rest is tag
    char *data_ = nullptr; // data_[0] is the lower end
    int bits_;   // record how many bits should be used, maybe more bits are allocated
    int bytes;      // how many bytes to allocate, == ceiling(bits_/8)
    bool hasDirty_; // if write_back, then true

    CacheLineMeta() {
    }

    ~CacheLineMeta() {
        delete[]data_;
    }

    void init(int bits, bool hasDirty = false) {
        hasDirty_ = hasDirty;
        bits_ = bits;
        bytes = bits / 8;
        if (bits % 8 != 0)
            bytes++;
        data_ = new char[bytes]();
    }

    // return the tag in this line
    size_t getTag() {
        size_t buf = 0;
        memcpy(&buf, data_, bytes);
        if (hasDirty_) {    // don't need valid and dirty bits
            buf = buf >> 2u;
        } else {
            buf = buf >> 1u;
        }
        return buf;
    }

    // set tag for this line, return the old tag
    size_t setTag(size_t value) {
        // old values
        size_t t = getTag();
        bool d;
        if (hasDirty_)
            d = getDirty();
        bool v = getValid();

        if (hasDirty_) {
            value = value << 2u;
        } else {
            value = value << 1u;
        }
        memcpy(data_, &value, bytes);   // not all bits of value are used
        if (hasDirty_) setDirty(d);    //restore dirty and valid bits
        setValid(v);
        return t;
    }

    // set the dirty bit
    bool setDirty(bool on) {
        if (!hasDirty_)
            return false;

        assert(hasDirty_);
        bool old = getDirty();
        if (on)
            data_[0] |= 0x2; // 0000 0010
        else
            data_[0] &= 0xFD; // 1111 1101
        return old;
    }

    // set the dirty bit, return the old dirty bit
    bool getDirty() {
        if (!hasDirty_)
            return false;
        assert(hasDirty_);
        return (data_[0] & 0x2); // second lowest bit
    }

    // set the valid bit, return the old valid bit
    bool setValid(bool value) {
        bool old = getValid();
        if (value == true)
            data_[0] |= 0x1; // 0000 0001
        else
            data_[0] &= 0xFE; // 1111 1110
        return old;
    }

    // get the valid bit
    bool getValid() {
        // valid is always the lowest bit
        return (data_[0] & 0x1);
    }
};

class Cache {
private:
    Option option;
    std::vector<Op> operations;
    lruStack *lru_ = nullptr;
    BinaryTree *binaryTrees_ = nullptr;
    CacheLineMeta *meta;

    void log_hit(bool isRead, bool hit) {
        if (hit) {
            if (isRead) {
                option.read_hit_count++;
            } else {
                option.write_hit_count++;
            }
            option.hit_count++;
        } else {
            if (isRead) {
                option.read_miss_count++;
            } else {
                option.write_miss_count++;
            }
            option.miss_count++;
        }
    }


public:
    static const size_t cache_size = 128 * 1024; // 128KB
    Cache(std::vector<Op> &ops, const Option &opt);

    ~Cache() {
        delete[] meta;
        delete[] lru_;
    }

    void run();

    bool cached(Address a, size_t &line_id);

    void moveIn(Address a);

    void updateMeta(Address a, size_t line_id, bool hit);

    void report();
};

#endif // CACHE_HPP
