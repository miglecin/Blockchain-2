#include "hash.h"
#include <sstream>
#include <iomanip>
#include <utility>

//bubble sort (rikiuoja pagal baito reiksme) su hash skaiciavimu per swap'us (256 bit)
std::array<uint32_t, 8> bubble_sort_and_hash(std::vector<char>& arr, std::array<uint32_t, 8> seed) {
    int n = (int)arr.size();

    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - 1 - i; ++j) {
            if (arr[j] > arr[j+1]) {
                //kai sukeiciam elemntus, atnaujinam hash
                unsigned int a = (unsigned char)arr[j];
                unsigned int b = (unsigned char)arr[j+1];

                int idx = j % 8; //pasirenkam, kuri 32-bit bloka keisti
                seed[idx] = (seed[idx] << 5) + (seed[idx] >> 3) + (a * 17 + b * 31 + j * 13);

                std::swap(arr[j], arr[j+1]);
            }
        }
    }
    return seed;
}

//salt (16 baitu)
std::vector<uint8_t> make_salt(const std::string& msg) { 
    std::vector<uint8_t> salt(16, 0); //sukuriam tusc salt
    for (size_t i = 0; i < msg.size(); i++) {
        //kiekviena raida imaisom i viena is 16 baitu (ASCII + pozicija*13), 0xFF kad nevirsytu 255
        salt[i % 16] = (salt[i % 16] + (uint8_t)msg[i] + (i * 13)) & 0xFF;
    }
    return salt;
}

//hex spausdinimas
std::string bytes_to_hex(const std::vector<uint8_t>& v) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t b : v) ss << std::setw(2) << (int)b;
    return ss.str();
}

//hash pavertimas i hex
std::string hash_to_hex(const std::array<uint32_t, 8>& h) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint32_t part : h) {
        ss<< std::setw(8) << part;
    }
    return ss.str();
}