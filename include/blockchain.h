#pragma once
#include "models.h"
#include <deque>
#include <vector>
#include <string>
#include <unordered_map>

struct User {
    std::string name;   // vartotojo vardas (pvz. user_0)
    std::string pubkey; // viesasis raktas (hash nuo vardo)
    uint64_t balance;   // pradinis balansas (tik pradinei UTXO generacijai)
};

// kandidatinis blokas (v0.2)
struct Candidate {
    std::vector<Transaction> txs; // transakcijos (0 = coinbase)
    BlockHeader header;           // header be nonce
};

class Blockchain {
public:
    explicit Blockchain(std::string diff_prefix = "000");

    // UTXO: sukuria n_users naudotoju ir kiekvienam 1 pradine UTXO
    void init_users(size_t n_users);

    // UTXO: sugeneruoja n_txs transakciju i mempool (naudojant laikina utxo vaizda be konfliktu)
    void init_transactions(size_t n_txs);

    // kasyba v0.1 (vienas kandidatas, be laiko limito) – palikta suderinamumui
    bool mine_next_block(size_t block_size);

    // kasyba v0.2 (keli kandidatai, laiko limitas ms)
    bool mine_next_block_v2(size_t block_size, size_t num_candidates, uint64_t max_ms);

    // skaitymui
    const std::vector<Block>& chain() const { return chain_; }
    const std::deque<Transaction>& mempool() const { return mempool_; }
    const Block* get_block_by_height(size_t height) const;
    const Transaction* get_transaction_by_id(const std::string& tx_id) const;

    // spausdinimui
    void print_transaction(const Transaction& tx) const;
    void print_block(const Block& b, size_t height) const;

    // diagnostika / REPL
    const std::vector<User>& users() const { return users_; }
    bool get_balance(const std::string& owner, uint64_t& out) const; // pagal pubkey arba "miner_0"
    void print_user(size_t idx) const;

private:
    // UTXO utils
    bool verify_block_txs(const std::vector<Transaction>& txs) const; // inputs exist, no double-spend, sum in >= sum out
    bool apply_block_state(const Block& b);                            // pritaiko UTXO ir atnaujina balances_
    void recompute_balances();                                         // perskaiciuoja balances_ is utxo_set

    // mining utils
    Candidate build_candidate_from_front(size_t block_size) const;
    bool try_mine_header(BlockHeader& h, std::string& out_hash, uint64_t max_ms) const;

    // hash utils
    std::string calc_tx_id(const Transaction& t) const;      // ID is vin/vout
    std::string merkle_root(std::vector<std::string> ids) const;
    std::string serialize_header(const BlockHeader& h) const;
    std::string hash_header(const BlockHeader& h) const;
    bool valid_pow(const std::string& hex) const;

private:
    // grandine ir mempool
    std::vector<Block> chain_;
    std::deque<Transaction> mempool_;

    // naudotojai
    std::vector<User> users_;

    // UTXO rinkinys: Outpoint -> TxOut
    std::unordered_map<Outpoint, TxOut, OutpointHash> utxo_set_;

    // REPL patogumui: owner(pubkey ar "miner_0") -> balansas (sumuojamas is utxo_set_)
    std::unordered_map<std::string, uint64_t> balances_;

    // PoW sunkumas
    std::string difficulty_;
};
