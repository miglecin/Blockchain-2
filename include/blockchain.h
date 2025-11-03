#pragma once
#include "models.h"
#include <deque>
#include <vector>
#include <string>
#include <unordered_map>

// vieno naudotojo info (v0.2)
struct User {
    std::string name;    // vartotojo vardas (pvz. user_0)
    std::string pubkey;  // viesasis raktas (hash nuo vardo)
    uint64_t balance;    // pradinis balansas
};

// paprasta kandidatinio bloko struktura (v0.2)
struct Candidate {
    std::vector<Transaction> txs;  // transakcijos siame kandidate (pirmoje vietoje bus coinbase)
    BlockHeader header;            // sukomplektuotas header be nonce (nonce bus ieskomas PoW metu)
};

class Blockchain {
public:
    // sukuria grandine su nurodytu sunkumu (pvz. "000")
    explicit Blockchain(std::string diff_prefix = "000");

    // v0.2: sugeneruoja n_users naudotoju ir ju balansus
    void init_users(size_t n_users);

    // sugeneruoja n_txs transakciju i mempool (naudoja users sarasa)
    void init_transactions(size_t n_txs);

    // v0.1 kasyba (vienas kandidatas, be laiko limito) – palikta suderinamumui
    bool mine_next_block(size_t block_size);

    // v0.2 kasyba: keli kandidatai ir laiko limitas (ms) vienai bangai
    bool mine_next_block_v2(size_t block_size, size_t num_candidates, uint64_t max_ms);

    // gauti pilna grandine (read-only)
    const std::vector<Block>& chain() const { return chain_; }

    // gauti mempool (read-only)
    const std::deque<Transaction>& mempool() const { return mempool_; }

    // rasti bloka pagal auksti (0 = genesis, 1 = pirmas iskastas blokas, ...)
    const Block* get_block_by_height(size_t height) const;

    // rasti transakcija pagal jos ID (hash) – iesko tiek chain'e, tiek mempool'e
    const Transaction* get_transaction_by_id(const std::string& tx_id) const;

    // prieiga (read-only)
    const std::vector<User>& users() const { return users_; }

    // gauti balansa pagal pubkey; grazina true jei rado
    bool get_balance(const std::string& pubkey, uint64_t& out) const;

    // patogumui: atspausdinti naudotoja pagal indeksa
    void print_user(size_t idx) const;

    // graziai isspausdina transakcija / bloka (block explorer stilius)
    void print_transaction(const Transaction& tx) const;
    void print_block(const Block& b, size_t height) const;

private:
    // pagalbiniai hash ir validacijos metodai
    std::string calc_tx_id(const Transaction& t) const;                 // deterministinis tx_id
    std::string calc_txs_hash(const std::vector<Transaction>& txs) const; // palikta suderinamumui (v0.1)
    std::string merkle_root(std::vector<std::string> leaves) const;     // v0.2: tikras merkle root
    std::string serialize_header(const BlockHeader& h) const;           // header -> tekstas
    std::string hash_header(const BlockHeader& h) const;                // HashAdapter nuo header
    bool valid_pow(const std::string& hex) const;                       // tikrina, ar hash prasideda sunkumo prefiksu

    // v0.2: transakciju tikrinimas pries kasima (balansai + tx_id)
    bool verify_block_txs(const std::vector<Transaction>& txs) const;

    // v0.2: kandidato formavimas is mempool priekio (neisimu is mempool realiai)
    Candidate build_candidate_from_front(size_t block_size) const;

    // v0.2: PoW su laiko limitu; grazina true jei rado nonce ir out_hash
    bool try_mine_header(BlockHeader& h, std::string& out_hash, uint64_t max_ms) const;

    // v0.2: pritaiko balansu busena po sekmingo bloko (sender -, receiver +, reward +)
    bool apply_block_state(const Block& b);

private:
    // grandines duomenys
    std::vector<Block> chain_;               // grandines blokai (0 = genesis)
    std::deque<Transaction> mempool_;       // patvirtinimo laukiancios transakcijos (FIFO)
    std::vector<User> users_;               // visi sugeneruoti naudotojai
    std::unordered_map<std::string, uint64_t> balances_; // pubkey -> balansas (dabartine busena)
    std::string difficulty_;                // PoW sunkumo prefiksas (pvz. "000")
};
