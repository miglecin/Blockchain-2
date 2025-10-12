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

    const std::vector<Block>& chain() const { return chain_; }
    const std::deque<Transaction>& mempool() const { return mempool_; }

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
