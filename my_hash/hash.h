#ifndef HASH_H
#define HASH_H

#include <string>
#include <vector>
#include <array>

//bubble sort su hash skaiciavimu
std::array<uint32_t, 8> bubble_sort_and_hash(std::vector<char>& arr, std::array<uint32_t, 8> seed);

//salt is teksto
std::vector<uint8_t> make_salt(const std::string& msg);

//baitu vekt i hex string
std::string bytes_to_hex(const std::vector<uint8_t>& v);

//hash (8x32 bit) i hex sring
std::string hash_to_hex(const std::array<uint32_t, 8>& h);

#endif
