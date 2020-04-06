//
// Created by Sam on 2020/4/6.
//

#include "Cache.hpp"
#include <cstdlib>
#include <cstring>

void lruStack::init(Option &opt) {
    p_option = &opt;
    if (opt.associative == way4) {
        // log2(4) * 4 = 8  --> one byte
        opt.lru_frame_bits_count = 2;
        data_ = new char[1]();
        bytes = 1;
        stackFrameMask = 0x3;  // 0000 0011
    } else if (opt.associative == way8) {
        // log2(8) * 8 = 24 --> three bytes
        opt.lru_frame_bits_count = 3;
        data_ = new char[3]();
        bytes = 3;
        stackFrameMask = 0x7;  // 0000 0111
    } else if (opt.associative == full) {
        int bits = log2_(opt.block_count);
        // no-align-impl

//        opt.lru_frame_bits_count = bits;
//        bytes = bits * opt.block_count / 8;
//        data_ = new char[bytes]();
//        for (int i = 0; i < bits; ++i) {
//            stackFrameMask = (stackFrameMask << 1u) + 1;
//        }

        // 2-byte-align
        // every stack frame will be stored in 2 bytes, since bits used
        // is 11 bit, 12 bit, 14 bit, when block_size = 8B, 32B, 64B
        opt.lru_frame_bits_count = 16;
        bytes = 2 * opt.block_count; // 2 bytes every block
        data_ = new char[bytes]();
    }
}

void lruStack::makeTop(size_t local_block_id, bool hit) {
    if (p_option->associative == way4 || p_option->associative == way8) {
        int n_blocks = p_option->block_count / p_option->group_count;

        if (hit) {
            int i;
            for (i = 0; i < n_blocks; ++i) {
                size_t tag = getTag(i);
                if (tag == local_block_id) {  // found
                    break;
                }
            }
            assert(i < n_blocks);
            for (; i > 0; i--) {
                setTag(i, getTag(i - 1)); // push down by one
            }
            setTag(0, local_block_id);   // make top
        } else {
            for (int i = n_blocks - 1; i > 0; i--) {
                setTag(i, getTag(i - 1)); // push down by one
            }
            setTag(0, local_block_id);   // make top
        }
    } else if (p_option->associative == full) {
        assert(p_option->group_count == 1);
        int n_blocks = p_option->block_count / p_option->group_count;
        if (hit) {
            int i;
            for (i = 0; i < n_blocks; ++i) {
                size_t tag = getTag(i);
                if (tag == local_block_id) {  // found
                    break;
                }
            }
            assert(i < n_blocks);
            std::memmove(data_ + 2, data_, i * 2);
            memcpy(data_, &local_block_id, 2);// make top
        } else {
            std::memmove(data_ + 2, data_, (n_blocks - 1) * 2);
            memcpy(data_, &local_block_id, 2);// make top
        }
        return;
    } else {
        assert(false);

    }
}

//get the i th element from top, each element b
size_t lruStack::getTag(int i) {
    if (p_option->associative == way4) {
//        size_t ret = 0;
//        size_t bit_start_index = i * p_option->lru_frame_bits_count;
//        size_t byte_start_index = bit_start_index / 8;
//        size_t remain = bit_start_index % 8;
//        size_t byte_count = (remain + p_option->lru_frame_bits_count) / 8;
//        if ((remain + p_option->lru_frame_bits_count) % 8 != 0)
//            byte_count++;
//
//        memcpy(&ret, data_ + byte_start_index, byte_count);
//        ret = ret >> remain;                // lower end is trash bits
//        ret = ret & stackFrameMask; // set higher unused to 0
//        return ret;
        assert(i < 4);
        size_t ret = 0;
        memcpy(&ret, data_, 1);     // one bytes used
        ret = ret & (stackFrameMask << (i * 2));
        ret = ret >> (i * 2);
        assert(ret < 4);

        return ret;
    } else if (p_option->associative == way8) {
        assert(i < 8);
        size_t ret = 0;
        memcpy(&ret, data_, 3);     // three bytes used
        ret = ret & (stackFrameMask << (i * 3));
        ret = ret >> (i * 3);
        assert(ret < 8);
        return ret;
    } else if (p_option->associative == full) {
        assert(i < p_option->block_count);
        size_t ret = 0;
        memcpy(&ret, data_ + 2 * i, 2);
        return ret;
    } else {
        assert(false);
        return 0;
    }
}

void lruStack::setTag(int i, size_t tag) {
    if (p_option->associative == way4) {
//        size_t bit_start_index = i * p_option->lru_frame_bits_count;
//        size_t byte_start_index = bit_start_index / 8;
//        size_t remain = bit_start_index % 8;     // which bit to start in the first byte
//        size_t byte_count = (remain + p_option->lru_frame_bits_count) / 8;
//        size_t end_remain = (remain + p_option->lru_frame_bits_count) % 8; // how many bits used in the last byte
//        if (end_remain != 0)
//            byte_count++;
//
//        char first = data_[byte_start_index];
//        char last = data_[byte_start_index + byte_count - 1];
//
//        tag = tag << remain;
//        memcpy(data_ + byte_start_index, &tag, byte_count);
//        unsigned char mask = 0;
//        for (int ii = 0; ii < remain; ++ii) {
//            mask = (mask << 1u) + 1;                //  like 0000 0111, if remain == 3
//        }
//        data_[byte_start_index] |= (first & mask);
//
//        mask = 0xFF;
//        for (int ii = 0; ii < end_remain; ++ii) {
//            mask = (mask << 1u);                    //  like 1111 1000, if end_remain == 3
//        }
//        data_[byte_start_index + byte_count - 1] |= (last & mask);
        assert(i < 4);
        assert(tag < 4);
        char mask = 0x3 << (i * 2);
        char buf = data_[0] & (~mask);    // make those two bits 0
        buf |= tag << (i * 2);
        data_[0] = buf;
        assert(getTag(i) == tag);
    } else if (p_option->associative == way8) {
        assert(i < 8);
        assert(tag < 8);
        size_t mask = 0x7 << (i * 3);
        size_t buf = 0;
        memcpy(&buf, data_, 3);
        buf = buf & (~mask);
        buf |= tag << (i * 3);
        memcpy(data_, &buf, 3);
        assert(getTag(i) == tag);
    } else if (p_option->associative == full) {
        assert(i < p_option->block_count);
        memcpy(data_ + 2 * i, &tag, 2);
    } else {
        assert(false);
    }

}

void Cache::moveIn(Address a) {
    size_t tag = a.addr >> (option.offset_bit_count + option.index_bit_count);
    size_t index = (a.addr & option.index_mask) >> option.offset_bit_count;
    if (option.associative == direct) {
//            size_t tag = a.addr >> (offset_bit_count + index_bit_count);
//            size_t index = (a.addr & index_mask) >> offset_bit_count;
        meta[index].setTag(tag);
        meta[index].setValid(true);
    } else {
        // 4-way, 8-way, fully-connected
        // check if there is empty space
        int blocks_in_group = option.block_count / option.group_count;
        for (int i = 0; i < blocks_in_group; ++i) {
            if (!meta[index * blocks_in_group + i].getValid()) {    // found empty space
                meta[index * blocks_in_group + i].setTag(tag);
                meta[index * blocks_in_group + i].setValid(true);
                if (option.replacement == binaryTree) {
                    binaryTrees_[index].access(i);
                } else if (option.replacement == randomReplace) {
                    // no need to update meta
                } else if (option.replacement == lru) {
                    // update lru meta
                    lru_[index].makeTop(i, false);
                }
                return;
            }
        }
        // no empty space, should replace
        if (option.replacement == binaryTree) {
            size_t block_id_to_evict = binaryTrees_[index].get_evict();
            meta[index * blocks_in_group + block_id_to_evict].setTag(tag);
            meta[index * blocks_in_group + block_id_to_evict].setValid(true);
            meta[index * blocks_in_group + block_id_to_evict].setDirty(false);
            binaryTrees_[index].access(block_id_to_evict);
        } else if (option.replacement == randomReplace) {
            int id = std::rand() % blocks_in_group;
            meta[index * blocks_in_group + id].setTag(tag);
            meta[index * blocks_in_group + id].setValid(true);
            meta[index * blocks_in_group + id].setDirty(false);
        } else if (option.replacement == lru) {
            size_t block_id_to_evict = lru_[index].bottom();
            lru_[index].makeTop(block_id_to_evict, false);
            meta[index * blocks_in_group + block_id_to_evict].setTag(tag);
            meta[index * blocks_in_group + block_id_to_evict].setValid(true);
            meta[index * blocks_in_group + block_id_to_evict].setDirty(false);
        }
    }
}

// check if the address is in cache,
// if yes, the cache line id will be stored in line_id,
// else line_id will not be modified
// this function does not modify the meta data
// any modification to meta data should be done in moveIn() or updateMeta()
bool Cache::cached(Address a, size_t &line_id) {
    size_t tag = a.addr >> (option.offset_bit_count + option.index_bit_count);

    // In associative cases, index is the group id,
    // In direct case, index is the block id
    size_t index = (a.addr & option.index_mask) >> option.offset_bit_count;
    if (option.associative == full) {
        for (size_t i = 0; i < option.block_count; i++) {
            if (meta[i].getValid() && meta[i].getTag() == tag) {    // check all blocks
                line_id = i;
                return true;
            }
        }
    } else if (option.associative == direct) {
        if (meta[index].getValid() && meta[index].getTag() == tag) {
            line_id = index;
            return true;
        } else {
            return false;
        }
    } else if (option.associative == way4) {

        for (size_t i = 0; i < 4; i++) {
            if (meta[index * 4 + i].getValid() && meta[index * 4 + i].getTag() == tag) {
                line_id = index * 4 + i;
                return true;
            }
        }
        return false;
    } else { // option.associative == way8;

        for (size_t i = 0; i < 8; i++) {
            if (meta[index * 8 + i].getValid() && meta[index * 8 + i].getTag() == tag) {
                line_id = index * 8 + i;
                return true;
            }
        }
        return false;
    }
    return false;
}

void Cache::updateMeta(Address a, size_t line_id, bool hit) {
    if (option.associative == direct)return;
    if (hit) {
//        size_t tag = a.addr >> (option.offset_bit_count + option.index_bit_count);
        size_t index = (a.addr & option.index_mask) >> option.offset_bit_count;
        int blocks_in_group = option.block_count / option.group_count;

        if (option.replacement == binaryTree) {
            binaryTrees_[index].access(line_id % blocks_in_group);
        } else if (option.replacement == randomReplace) {
            // no need to update
        } else if (option.replacement == lru) {
            lru_[index].makeTop(line_id % blocks_in_group, true);
        }
    } else {
        // if not hit, should call moveIn(), and meta will be updated there,
        // so code here should never be executed
        assert(false);
    }
}

void Cache::report() {
    std::cout << "Block size: " << option.blockSize << std::endl;
    std::cout << "Write policy: " << (option.writeBack ? "write-back" : "write-through") << " + ";
    std::cout << (option.allocate ? "write-allocate" : "write-no-allocate") << std::endl;
    std::cout << "Associative: ";
    if (option.associative == full) {
        std::cout << "Fully-connected" << std::endl;
    } else if (option.associative == direct) {
        std::cout << "direct" << std::endl;
    } else if (option.associative == way4) {
        std::cout << "4-way" << std::endl;
    } else if (option.associative == way8) {
        std::cout << "8-way" << std::endl;
    }
    std::cout << "Replacement: ";
    if (option.replacement == lru) {
        std::cout << "LRU" << std::endl;
    } else if (option.replacement == binaryTree) {
        std::cout << "Binary tree" << std::endl;
    } else if (option.replacement == randomReplace) {
        std::cout << "Random" << std::endl;
    }
    std::cout << "Miss count: " << option.miss_count
              << "(" << option.read_miss_count << " read + " << option.write_miss_count << " write)" << std::endl;
    std::cout << "Hit count: " << option.hit_count << "(" << option.read_hit_count << " read + "
              << option.write_hit_count << " write)"
              << std::endl;
    assert(operations.size()==option.hit_count+option.miss_count);
    assert(option.hit_count==option.read_hit_count+option.write_hit_count);
    assert(option.miss_count==option.read_miss_count+option.write_miss_count);

    std::cout << "Total access count: " << operations.size() << std::endl;
    std::cout << "Cache miss rate: "
              << 100.0 * double(option.miss_count) / double(option.miss_count + option.hit_count) << "%"
              << std::endl;
}

void Cache::run() {
    for (size_t i = 0; i < operations.size(); ++i) {
        auto &o = operations[i];
        size_t line_id;
        bool inCache = cached(o.address, line_id);
        log_hit(o.read, inCache);
        if (!inCache) {
            if (o.read || (!o.read && option.allocate)) {
                moveIn(o.address);
            }
        } else {
            updateMeta(o.address, line_id, true);
        }
    }
}

Cache::Cache(std::vector<Op> &ops, const Option &opt) : option(opt),
                                                        operations(ops) {
    option.block_count = cache_size / opt.blockSize;
    if (option.associative == way8) {
        option.group_count = option.block_count / 8;
    } else if (option.associative == way4) {
        option.group_count = option.block_count / 4;
    } else if (option.associative == direct) {
        option.group_count = option.block_count;
    } else { // fully connected
        option.group_count = 1;
    }
    option.offset_bit_count = log2_(option.blockSize);
    option.index_bit_count = log2_(option.group_count);
    option.tag_bit_count = 64 - option.index_bit_count - option.offset_bit_count;

    option.dirty_bit_count = opt.writeBack ? 1 : 0;

    for (int i = 0; i < option.offset_bit_count; ++i) {
        option.offset_mask = (option.offset_mask << 1u) + 1;
    }

    for (int i = 0; i < option.index_bit_count; ++i) {
        option.index_mask = (option.index_mask << 1u) + 1;
    }
    option.index_mask = option.index_mask << option.offset_bit_count;

    for (int i = 0; i < option.tag_bit_count; ++i) {
        option.tag_mask = (option.tag_mask << 1u) + 1;
    }
    option.tag_mask = option.tag_mask << (option.offset_bit_count + option.index_bit_count);

    meta = new CacheLineMeta[option.block_count];
    for (size_t i = 0; i < option.block_count; i++) {
        meta[i].init(option.tag_bit_count + option.valid_bit_count + option.dirty_bit_count,
                     option.writeBack);
    }

    // init replacement meta data according to policy
    if (option.replacement == lru) {
        lru_ = new lruStack[option.group_count];
        for (int i = 0; i < option.group_count; ++i) {
            lru_[i].init(option);
        }
    } else if (option.replacement == binaryTree) {
        binaryTrees_ = new BinaryTree[option.group_count];
        for (int i = 0; i < option.group_count; ++i) {
            binaryTrees_[i].init(option);
        }
    }// no meta data needed for random replacement
}

void BinaryTree::init(Option &opt) {
    p_option = &opt;
    if (opt.associative == way4) {
        data_ = new char[1]();
        bits_ = 3;
//        bytes = 1;

    } else if (opt.associative == way8) {

        data_ = new char[1]();
        bits_ = 7;
//        bytes = 1;

    } else if (opt.associative == full) {
        // binary is not supported for (opt.associative == full)
        // and no test is required
//        bits_ = opt.block_count -1;
//        bytes = (bits_+1)/ 8;
//        data_ = new char[bytes]();
        assert(false);
    }
}

void BinaryTree::access(size_t local_block_id) {
    if (p_option->associative == way4) {
        assert(local_block_id < 4);
        switch (local_block_id) {
            case 0:
                data_[0] |= (MASK0 | MASK1);  // B0=1, B1=1
                break;
            case 1:
                data_[0] |= MASK0;          // B0=1
                data_[0] &= ~MASK0;         // B1=0
                break;
            case 2:
                data_[0] &= ~MASK0;          // B0=0
                data_[0] |= MASK2;         // B2=1
                break;
            case 3:
                data_[0] &= ~MASK0;          // B0=0
                data_[0] &= ~MASK2;          // B2=0
                break;
            default:
                break;
        }
    } else if (p_option->associative == way8) {
        assert(local_block_id < 8);
        switch (local_block_id) {
            case 0:
                data_[0] |= MASK0;  // B0=1
                data_[0] |= MASK1;// B1=1
                data_[0] |= MASK3;// B3=1
                break;
            case 1:
                data_[0] |= MASK0;  // B0=1
                data_[0] |= MASK1;// B1=1
                data_[0] &= ~MASK3;  // B0=0
                break;
            case 2:
                data_[0] |= MASK0;  // B0=1
                data_[0] &= ~MASK1;  // B1=0
                data_[0] |= MASK4;  // B4=1
                break;
            case 3:
                data_[0] |= MASK0;  // B0=1
                data_[0] &= ~MASK1;  // B1=0
                data_[0] &= ~MASK4;  // B4=0
                break;
            case 4:
                data_[0] &= ~MASK0;  // B0=0
                data_[0] |= MASK2;  // B2=1
                data_[0] |= MASK5;  // B5=1
                break;
            case 5:
                data_[0] &= ~MASK0;  // B0=0
                data_[0] |= MASK2;  // B2=1
                data_[0] &= ~MASK5;  // B5=0
                break;
            case 6:
                data_[0] &= ~MASK0;  // B0=0
                data_[0] &= ~MASK2;  // B2=0
                data_[0] |= MASK6;  // B6=1
                break;
            case 7:
                data_[0] &= ~MASK0;  // B0=0
                data_[0] &= ~MASK2;  // B2=0
                data_[0] &= ~MASK6;  // B6=0

                break;
            default:
                break;
        }


    } else {
        assert(false);
    }
}
