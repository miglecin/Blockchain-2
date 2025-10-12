#include "hash_adapter.h"

std::string HashAdapter::hash_bytes(const std::vector<uint8_t>& bytes) {
    std::string msg(bytes.begin(), bytes.end());
    auto salt = make_salt(msg);

    std::vector<char> data;
    data.reserve(salt.size() + bytes.size());
    for (uint8_t b : salt)  data.push_back(static_cast<char>(b));
    for (uint8_t b : bytes) data.push_back(static_cast<char>(b));

    uint8_t first_b = bytes.empty() ? 0 : bytes.front();
    uint8_t last_b  = bytes.empty() ? 0 : bytes.back();
    uint32_t L = static_cast<uint32_t>(bytes.size());

    std::array<uint32_t, 8> seed = {
        L * 123u,
        static_cast<uint32_t>(first_b) * 4567u,
        static_cast<uint32_t>(last_b) * 8910u,
        ((L << 16) ^ 0xDEADu),
        (0xAAAAAAAAu ^ L),
        (0x55555555u + L),
        (0xF0F0F0F0u ^ first_b),
        (0x0F0F0F0Fu ^ last_b)
    };

    auto h = bubble_sort_and_hash(data, seed);
    return hash_to_hex(h);
}

std::string HashAdapter::hash_string(const std::string& s) {
    return hash_bytes(std::vector<uint8_t>(s.begin(), s.end()));
}