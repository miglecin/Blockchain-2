#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>

//hash API (deklaracijos iš hash.h)
std::array<uint32_t, 8> bubble_sort_and_hash(std::vector<uint8_t>& arr, std::array<uint32_t, 8> seed);
std::vector<uint8_t> make_salt(const std::string& msg);
std::string hash_to_hex(const std::array<uint32_t, 8>& h);

class HashAdapter {
public:
    // Įvairiems duomenims
    static std::string hash_bytes(const std::vector<uint8_t>& bytes);
    // Patogu tekstams
    static std::string hash_string(const std::string& s);
};
