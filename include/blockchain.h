#pragma once
#include "models.h"
#include <deque>
#include <vector>
#include <string>
#include <unordered_map>

// paprasta vartotojo info struktura
struct User {
    std::string name;    // vartotojo vardas (pvz. user_0)
    std::string pubkey;  // viesasis raktas (hash nuo vardo)
    uint64_t balance;    // tik suvestinei (gaunama is UTXO)
};

// kandidatinis blokas
struct Candidate {
    std::vector<Transaction> txs;
    BlockHeader header;
};

class Blockchain {
public:
    explicit Blockchain(std::string diff_prefix = "000");

    // sugeneruoti vartotojus ir pradini UTXO rinkini
    void init_users(size_t n_users);

    // sugeneruoti transakcijas i mempool (UTXO, be konfliktu)
    void init_transactions(size_t n_txs);

    // kasyba
    bool mine_next_block(size_t block_size); // v0.1
    bool mine_next_block_v2(size_t block_size, size_t num_candidates, uint64_t max_ms); // v0.2
    bool mine_next_block_v2_parallel(size_t block_size, size_t num_candidates, uint64_t max_ms);

    // skaitymui (REPL)
    const std::vector<Block>& chain() const { return chain_; }
    const std::deque<Transaction>& mempool() const { return mempool_; }
    const Block* get_block_by_height(size_t height) const;
    const Transaction* get_transaction_by_id(const std::string& tx_id) const;

    void print_transaction(const Transaction& tx) const;
    void print_block(const Block& b, size_t height) const;

    // suvestines patogumui
    bool get_balance(const std::string& owner, uint64_t& out) const;
    void print_user(size_t idx) const;

private:
    // pagalbiniai
    std::string calc_tx_id(const Transaction& t) const;
    std::string merkle_root(std::vector<std::string> ids) const;
    std::string serialize_header(const BlockHeader& h) const;
    std::string hash_header(const BlockHeader& h) const;
    bool        valid_pow(const std::string& hex) const;

    bool verify_block_txs(const std::vector<Transaction>& txs) const;
    bool apply_block_state(const Block& b);
    void recompute_balances();

    Candidate build_candidate_from_front(size_t block_size) const;
    bool try_mine_header(BlockHeader& h, std::string& out_hash, uint64_t max_ms) const;

private:
    // busena
    std::deque<Transaction> mempool_;
    std::vector<Block>      chain_;
    std::string             difficulty_;

    // UTXO ir suvestines
    std::unordered_map<Outpoint, TxOut, OutpointHash> utxo_set_;
    std::unordered_map<std::string, uint64_t>         balances_;
    std::vector<User>                                  users_;
};
