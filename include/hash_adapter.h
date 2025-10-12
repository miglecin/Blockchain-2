#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include "hash.h"


class HashAdapter {
public:
    static std::string hash_bytes(const std::vector<uint8_t>& bytes);
    static std::string hash_string(const std::string& s);
};
