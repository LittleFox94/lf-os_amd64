#ifndef _BITSET_HELPERS_H_INCLUDED
#define _BITSET_HELPERS_H_INCLUDED

#include <stddef.h>
#include <bitset>

template<size_t Bits>
struct bitset_helpers {
    /**
     * Finds the beginning of a streak of a least n unset bits in the given bitset.
     * \param bitset Bitset to search for a continuous streak of unset bits
     * \param n minimum length of streak to find
     * \returns index of first bit in the streak or Bits+1 if none found
     */
    static size_t find_continuous_unset(const std::bitset<Bits>& bitset, size_t n) {
        for(size_t candidate = 0; candidate < Bits - n; ++candidate) {
            bool success = true;
            for(size_t i = 0; i < n; ++i) {
                if(bitset.test(candidate + i)) {
                    success = false;
                    candidate += i;
                    break;
                }
            }

            if(success) {
                return candidate;
            }
        }

        // can we get here, with the check if we have enough continues
        // entries in this page on entry of this method?
        return Bits + 1;
    }

    static void set_range(std::bitset<Bits>& bitset, size_t start, size_t n, bool value = true) {
        for(size_t i = 0; i < n; ++i) {
            bitset.set(start + i, value);
        }
    }
};

#endif
