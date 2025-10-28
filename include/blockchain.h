#pragma once
#include "models.h"
#include <deque>
#include <vector>
#include <string>

class Blockchain {
public:
    explicit Blockchain(std::string diff_prefix = "000");
    void init_transactions(size_t n_txs);
    bool mine_next_block(size_t block_size);

     // gauti pilna grandine (read-only)
    const std::vector<Block>& chain() const { return chain_; }
    // gauti mempool (read-only)
    const std::deque<Transaction>& mempool() const { return mempool_; }

    // rasti bloka pagal auksti (0 = genesis, 1 = pirmas iskastas blokas, ...)
    const Block* get_block_by_height(size_t height) const;
    // rasti transakcija pagal jos ID (hash)
    // iesko tiek chain'e, tiek mempool'e
    const Transaction* get_transaction_by_id(const std::string& tx_id) const;
    void print_transaction(const Transaction& tx) const;
    // graziai isspausdina bloko informacija (block explorer stilius)
    void print_block(const Block& b, size_t height) const;

private:
    std::string calc_tx_id(const Transaction& t) const;
    std::string calc_txs_hash(const std::vector<Transaction>& txs) const;
    std::string serialize_header(const BlockHeader& h) const;
    std::string hash_header(const BlockHeader& h) const;
    bool valid_pow(const std::string& hex) const;

private:
    //std::vector<User> users_;             
    std::deque<Transaction> mempool_;
    std::vector<Block> chain_;
    std::string difficulty_;
};
