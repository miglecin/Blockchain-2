#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

// outpoint nurodo konkretu isejima: (txid, index)
struct Outpoint {
    std::string txid;
    uint32_t index;
    bool operator==(const Outpoint& o) const { return index == o.index && txid == o.txid; }
};

// outpoint hash (naudojamas unordered_map)
struct OutpointHash {
    size_t operator()(const Outpoint& o) const {
        size_t h1 = std::hash<std::string>{}(o.txid);
        size_t h2 = std::hash<uint32_t>{}(o.index);
        return h1 ^ (h2 << 1);
    }
};

// vienas ivesties irasas (nukreipia i praeita isejima)
struct TxIn {
    Outpoint prev; // i kuri isejima rodo (cia demo – be signaturu)
};

// vienas isejimas: kam priklauso ir kiek verte
struct TxOut {
    std::string owner; // savininko pubkey arba specialus "miner_0"
    uint64_t value;    // kiek vienetu
};

// pilna transakcija (UTXO)
struct Transaction {
    std::vector<TxIn>  vin;   // ivestys
    std::vector<TxOut> vout;  // isejimai
    std::string tx_id;        // transakcijos identifikatorius (hash nuo vin/vout)
};

// bloko header
struct BlockHeader {
    std::string prev_block_hash;
    uint64_t    timestamp = 0;
    std::string version   = "v0.2-utxo";
    std::string txs_hash;      // merkle root
    uint64_t    nonce = 0;
    std::string difficulty = "000";
};

// blokas
struct Block {
    BlockHeader header;
    std::vector<Transaction> txs;
    std::string block_hash; // Hash(header)
};
