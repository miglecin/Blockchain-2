#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Transaction {
    std::string sender;     // siuntėjo adresas (pvz. "user_12")
    std::string receiver;   // gavėjo adresas (pvz. "user_87")
    uint64_t amount = 0;    // kiek siųsta (pvz. 250)
    uint64_t nonce  = 0;    // unikalus numeris, kad nebūtų duplikatų
    std::string tx_id;      // transakcijos identifikatorius (hash)
};

struct BlockHeader {
    std::string prev_block_hash; //hash ankstesnio bloko
    uint64_t    timestamp = 0;   //kada blokas sukurtas
    std::string version   = "v0.1";
    std::string txs_hash;       // v0.1: concat(tx_id) hash
    uint64_t    nonce = 0;      //PoW
    std::string difficulty = "000"; //kiek „nulinių“ reikia PoW uždavinyje
};

struct Block {
    BlockHeader header;
    std::vector<Transaction> txs; //visos transakcijos, kurios yra tame bloke
    std::string block_hash; //Hash(header)-bloko parašas
};
