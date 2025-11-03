#include "blockchain.h"
#include "hash_adapter.h"
#include "utils.h"
#include <algorithm>
#include <iostream>

//block explorer style printinimui
static void print_block_pretty(const Block& b, size_t height) {
    std::cout << "\n========================================\n";
    std::cout << " Block #" << height << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << " block_hash:      " << b.block_hash << "\n";
    std::cout << " prev_block_hash: " << b.header.prev_block_hash << "\n";
    std::cout << " difficulty:      " << b.header.difficulty << "\n";
    std::cout << " nonce:           " << b.header.nonce << "\n";
    std::cout << " timestamp:       " << b.header.timestamp << "\n";
    std::cout << " txs_in_block:    " << b.txs.size() << "\n";
    std::cout << " txs_hash:        " << b.header.txs_hash << "\n";
    std::cout << "----------------------------------------\n";

    //parodau pirmas kelias transakcijas
    size_t preview_count = std::min((size_t)5, b.txs.size());
    for (size_t i = 0; i < preview_count; i++) {
        const Transaction& tx = b.txs[i];
        std::cout << " tx[" << i << "]: "
                  << tx.sender << " -> " << tx.receiver
                  << " | amount=" << tx.amount
                  << " | nonce=" << tx.nonce
                  << " | id=" << tx.tx_id.substr(0, 16) << "...\n";
    }

    if (b.txs.size() > preview_count) {
        std::cout << " ... (" << (b.txs.size() - preview_count)
                  << " more txs not shown)\n";
    }

    std::cout << "========================================\n\n";
}

//bazinis bloko atlygis (50)
static const uint64_t BASE_BLOCK_REWARD = 50;

//paprasta halving taisykle kas 50 bloku
static uint64_t current_block_reward(size_t height) {
    size_t era = height / 50; // every 50 blocks, new "era"
    //bitinis poslinkis i desine sumazina atlygi: 50 -> 25 -> 12 -> ...
    uint64_t reward = BASE_BLOCK_REWARD >> era;
    if (reward == 0) reward = 1; //demo versijai atlygis niekada nenukrenta zemiau 1
    return reward;
}

Blockchain::Blockchain(std::string diff_prefix)
  : difficulty_(std::move(diff_prefix)) {
    //GENESIS (nekasamas)
    Block genesis;
    genesis.header.prev_block_hash = std::string(64, '0');
    genesis.header.timestamp = now_ts();
    genesis.header.difficulty = difficulty_;
    genesis.header.txs_hash = ""; //nera tx
    genesis.header.nonce = 0;
    genesis.block_hash = hash_header(genesis.header); //suskaičiuoji hash’ą nuo header (be kasimo, nes genesis paprastai nekasamas)
    chain_.push_back(std::move(genesis)); //istatau i chaina
    std::cout << "[chain] genesis block created (height=0)\n";
}

// v0.2: vartotoju generavimas su balansais
void Blockchain::init_users(size_t n_users) {
    users_.clear();
    balances_.clear();
    users_.reserve(n_users);
    for (size_t i = 0; i < n_users; ++i) {
        User u;
        u.name = "user_" + std::to_string(i);
        u.pubkey = HashAdapter::hash_string(u.name);
        u.balance = rand_u64(100, 1'000'000);
        users_.push_back(u);
        balances_[u.pubkey] = u.balance;
    }
    std::cout << "[users] generated " << users_.size() << " users\n";
}

std::string Blockchain::calc_tx_id(const Transaction& t) const {
    //sudedu i viena stringa
    std::string payload = t.sender + "|" + t.receiver + "|" + std::to_string(t.amount) + "|" + std::to_string(t.nonce); 
    return HashAdapter::hash_string(payload); //pritaikau savo hash
}

//TRANSAKCIJU GENERAVIMAS is users v0.2
void Blockchain::init_transactions(size_t n_txs) {
    mempool_.clear(); //isvalom mempool (jei kazkas buvo anksciau)
    if (users_.empty()) {
        std::cout << "[warn] users are empty, calling init_users(1000)\n";
        init_users(1000);
    }
    
    //sukuriam n_txs transakciju
   for (size_t i = 0; i < n_txs; ++i) {
        Transaction tx;
        size_t sidx = i % users_.size();
        size_t ridx = (i + 1) % users_.size();
        tx.sender   = users_[sidx].pubkey;
        tx.receiver = users_[ridx].pubkey;
        tx.amount   = rand_u64(1, 100);
        tx.nonce    = i;
        tx.tx_id    = calc_tx_id(tx);
        mempool_.push_back(tx);

        if ((i + 1) % 1000 == 0) {
            std::cout << "[mempool] generated " << (i + 1) << " / " << n_txs << " txs\n";
        }
    }
    std::cout << "[mempool] total " << mempool_.size() << " transactions ready for mining\n";
}

//concat hash (palikta suderinamumui; v0.2 naudos merkle_root)
std::string Blockchain::calc_txs_hash(const std::vector<Transaction>& txs) const {
    //sujungiu visų tx_id į vieną ilgą stringą, tada per HashAdapter
    std::string concat; concat.reserve(txs.size() * 64);
    for (auto& t : txs) concat += t.tx_id;
    return HashAdapter::hash_string(concat);
}

//merkle root is tx_id
std::string Blockchain::merkle_root(std::vector<std::string> leaves) const {
    if (leaves.empty()) return HashAdapter::hash_string("");
    while (leaves.size() > 1) {
        if (leaves.size() & 1) leaves.push_back(leaves.back());
        std::vector<std::string> next;
        next.reserve(leaves.size() / 2);
        for (size_t i = 0; i < leaves.size(); i += 2) {
            next.push_back(HashAdapter::hash_string(leaves[i] + leaves[i + 1]));
        }
        leaves.swap(next);
    }
    return leaves[0];
}

std::string Blockchain::serialize_header(const BlockHeader& h) const {
    //Paverčia header’į į vieną tekstinę eilutę, iš kurios skaičiuosiu hash
    std::string s;
    s += h.prev_block_hash;
    s += "|" + std::to_string(h.timestamp);
    s += "|" + h.version;
    s += "|" + h.txs_hash;
    s += "|" + std::to_string(h.nonce);
    s += "|" + h.difficulty;
    return s;
}

std::string Blockchain::hash_header(const BlockHeader& h) const {
    return HashAdapter::hash_string(serialize_header(h));
}

bool Blockchain::valid_pow(const std::string& hex) const {
    //Patikrina, ar hash prasideda tavo sunkumo prefiksu (pvz "000")
    return hex.rfind(difficulty_, 0) == 0; 
    //PoW TAISYKLE
}

//grazina bloka pagal jo auksti (0 = genesis blokas)
const Block* Blockchain::get_block_by_height(size_t height) const {
    if (height >= chain_.size()) return nullptr;
    return &chain_[height];
}

//iesko transakcijos pagal jos tx_id (search mined blocks, then mempool)
const Transaction* Blockchain::get_transaction_by_id(const std::string& tx_id) const {
    //iesko chaine pirmiausia
    for (const auto& b : chain_) {
        for (const auto& tx : b.txs) {
            if (tx.tx_id == tx_id) {return &tx;}
        }
    }
    //iesko mempoole
    for (const auto& tx : mempool_) {
        if (tx.tx_id == tx_id) {return &tx;}
    }
    return nullptr;
}

// pretty print a single transaction
void Blockchain::print_transaction(const Transaction& tx) const {
    std::cout << "tx_id:    " << tx.tx_id << "\n";
    std::cout << " sender:   " << tx.sender << "\n";
    std::cout << " receiver: " << tx.receiver << "\n";
    std::cout << " amount:   " << tx.amount << "\n";
    std::cout << " nonce:    " << tx.nonce << "\n";
}
// pretty print a block 
void Blockchain::print_block(const Block& b, size_t height) const {
    print_block_pretty(b, height);
}

//pritaikyti balansu busena po sekmingo bloko
bool Blockchain::apply_block_state(const Block& b) {
    for (const auto& tx : b.txs) {
        if (tx.sender == "Block_Reward") {
            balances_[tx.receiver] += tx.amount;
            continue;
        }
        auto itS = balances_.find(tx.sender);
        auto itR = balances_.find(tx.receiver);
        if (itS == balances_.end() || itR == balances_.end()) return false;
        if (itS->second < tx.amount) return false;
        itS->second -= tx.amount;
        itR->second += tx.amount;
    }
    return true;
}

//patikrinti, kad visos tx yra teisingos pries kasima
bool Blockchain::verify_block_txs(const std::vector<Transaction>& txs) const {
    // laikina balansu kopija, kad patikrintume nuosekliai
    std::unordered_map<std::string, uint64_t> tmp = balances_;
    for (const auto& tx : txs) {
        if (tx.sender == "Block_Reward") {
            tmp[tx.receiver] += tx.amount;
            continue;
        }
        // tikrinam tx_id
        if (tx.tx_id != calc_tx_id(tx)) return false;
        auto itS = tmp.find(tx.sender);
        auto itR = tmp.find(tx.receiver);
        if (itS == tmp.end() || itR == tmp.end()) return false;
        if (itS->second < tx.amount) return false;
        itS->second -= tx.amount;
        itR->second += tx.amount;
    }
    return true;
}

// suformuoja kandidata paemus transakcijas is mempool PRIEKIO, bet nepanaikina mempool
Candidate Blockchain::build_candidate_from_front(size_t block_size) const {
    Candidate c;
    // coinbase pirmoje vietoje
    Transaction reward_tx;
    reward_tx.sender   = "Block_Reward";
    reward_tx.receiver = "miner_0";
    reward_tx.amount   = current_block_reward(chain_.size());
    reward_tx.nonce    = now_ts();
    reward_tx.tx_id    = HashAdapter::hash_string(
        std::string("COINBASE|") + reward_tx.receiver + "|" +
        std::to_string(reward_tx.amount) + "|" +
        std::to_string(reward_tx.nonce)
    );

    c.txs.push_back(reward_tx);

    // pridekim dar block_size-1 transakciju is mempool priekio (snapshot)
    size_t taken = 0;
    for (const auto& tx : mempool_) {
        if (taken >= block_size - 1) break;
        c.txs.push_back(tx);
        ++taken;
    }

    // uzpildom header be nonce
    c.header.prev_block_hash = chain_.back().block_hash;
    c.header.timestamp       = now_ts();
    c.header.difficulty      = difficulty_;
    // v0.2: merkle root
    std::vector<std::string> ids; ids.reserve(c.txs.size());
    for (auto& t : c.txs) ids.push_back(t.tx_id);
    c.header.txs_hash        = merkle_root(std::move(ids));
    c.header.nonce           = 0;

    return c;
}

// oW su laiko limitu
bool Blockchain::try_mine_header(BlockHeader& h, std::string& out_hash, uint64_t max_ms) const {
    using clk = std::chrono::steady_clock;
    auto start = clk::now();
    size_t iters = 0;
    while (true) {
        ++h.nonce;
        out_hash = hash_header(h);
        if (valid_pow(out_hash)) return true;

        if ((++iters & 0x3FFFF) == 0) {
            std::cout << "[mining] nonce=" << h.nonce
                      << " hash=" << out_hash.substr(0,16) << "...\r" << std::flush;
            auto now = clk::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (ms >= (long long)max_ms) break;
        }
    }
    return false;
}

//---------------------------------
//KASIMAS PoW v0.1 (vienas kandidatas be laiko limito)
bool Blockchain::mine_next_block(size_t block_size) {
    //jei mempool tuscias - nieko nekasiam
    if (mempool_.empty()) return false;

    //1) paimam iki block_size transakciju is mempool priekio (FIFO)
    std::vector<Transaction> batch;
    batch.reserve(block_size); 
    for (size_t i=0; i<block_size && !mempool_.empty(); ++i) {
        batch.push_back(mempool_.front());
        mempool_.pop_front();
    }

    //coinbase transaction (block reward)
     Transaction reward_tx;
     reward_tx.sender   = "Block_Reward"; //specialus naudotojas – centrinis bankas
     reward_tx.receiver = "miner_0"; //mineris, KURIS GAUNA REWARDA
     reward_tx.amount   = current_block_reward(chain_.size()); //dinaminis rewardas
     reward_tx.nonce    = now_ts();  //unikalus
     reward_tx.tx_id    = HashAdapter::hash_string(
                                std::string("COINBASE|") +
                                reward_tx.receiver + "|" +
                                std::to_string(reward_tx.amount) + "|" +
                                std::to_string(reward_tx.nonce) );

     //parodau rewarda konsolej
    std::cout << "[reward] " << reward_tx.amount
              << " coins issued by Block_Reward to "
              << reward_tx.receiver << "\n";

     batch.insert(batch.begin(), reward_tx);//idedu rewarda i pirma transakcija siame bloke

    // log: starting to build a new block with N txs
    std::cout << "[block] forming new block with "
              << batch.size() << " txs\n";

    //2) paruosiam bloko antraste (header) kuri bus hashuojama
    BlockHeader h;
    h.prev_block_hash = chain_.back().block_hash; // nuoroda i ankstesni bloka
    h.timestamp       = now_ts();                 // dabartinis laikas (unix)
    h.difficulty      = difficulty_;              // pvz. "000"
    h.txs_hash        = calc_txs_hash(batch);     //visu siame bloke esanaciu tx hash
    h.nonce           = 0;                        //pradzioje nulis, veliau dideja

    //3) kasimo ciklas: didinam nonce kol hash prasideda norimu prefiksu
    std::string bh; //dabartinis bloko hash kandidatas
    size_t iters = 0; //kiek kartu bande
    do {
        ++h.nonce;
        bh = hash_header(h); //hash nuo header

        //kas tam tikra kieki (~260k) bandymu parodom progresa
        if ((++iters & 0x3FFFF) == 0) {
            std::cout << "[mining] nonce=" << h.nonce << " hash=" << bh.substr(0,16) << "...\r" << std::flush;
        }
    } while (!valid_pow(bh)); //kol hash neprasideda pvz. "000"

    std::cout << "\n[mined] block found! nonce=" << h.nonce << " hash=" << bh.substr(0,16) << "...\n";

    //4) sukuriam nauja bloka ir idedam i grandine
    Block b; b.header = h; b.txs = std::move(batch); b.block_hash = bh;
    chain_.push_back(std::move(b));

    // 5) santrauka: po to kai blokas pridedamas
    // parodome kiek transakciju buvo ideta i si bloka
    // ir koks dabar yra grandines aukstis (kiek bloku is viso)
    std::cout << "[block] applied: " << chain_.back().txs.size()
              << " txs added to block #"
              << (chain_.size() - 1)
              << "\n";

    std::cout << "[chain] height=" << chain_.size()
              << " mempool_left=" << mempool_.size()
              << "\n";

    // 6) pretty print full block info (explorer-style)
    // kvieciame su aukstis = chain_.size() - 1 (paskutinis blokas)
    print_block_pretty(chain_.back(), chain_.size() - 1);

    return true;
}


// v0.2 kasyba: keli kandidatai ir laiko limitas vienai bangai
bool Blockchain::mine_next_block_v2(size_t block_size, size_t num_candidates, uint64_t max_ms) {
    if (mempool_.empty()) return false;

    // sugeneruojam kandidatus (naudojam momentini mempool prieki)
    std::vector<Candidate> cands; cands.reserve(num_candidates);
    for (size_t i = 0; i < num_candidates; ++i) {
        cands.push_back(build_candidate_from_front(block_size));
        // nedidelis variacijos triukas: pakeisti timestamp, kad skirtingi header
        cands.back().header.timestamp += i;
    }

    // verifikuojam kiekvieno kandidato tx pries kasyma
    for (auto it = cands.begin(); it != cands.end(); ) {
        if (verify_block_txs(it->txs)) ++it;
        else it = cands.erase(it);
    }
    if (cands.empty()) {
        std::cout << "[block] no valid candidates (verification failed)\n";
        return false;
    }

    // bandome kasti kandidatus nuosekliai (emuliuojam daug kasikliu)
    // pirma sekmingai iskasta uzfiksuojama
    for (auto& cand : cands) {
        std::string bh;
        BlockHeader h = cand.header;
        bool ok = try_mine_header(h, bh, max_ms);
        if (!ok) {
            std::cout << "[mining] candidate timed out (no solution)\n";
            continue;
        }

        std::cout << "\n[mined] block found! nonce=" << h.nonce
                  << " hash=" << bh.substr(0,16) << "...\n";

        // jei radom, suformuojam bloka ir realiai isimam tx is mempool
        // isimam tik tiek, kiek is tiesu sudarem kandidate
        // pirma tx yra coinbase, jos mempoole nera
        size_t need = cand.txs.size() - 1;
        for (size_t i = 0; i < need && !mempool_.empty(); ++i) {
            mempool_.pop_front();
        }

        Block b;
        b.header = h;
        b.txs    = std::move(cand.txs);
        b.block_hash = bh;

        chain_.push_back(std::move(b));
        if (!apply_block_state(chain_.back())) {
            std::cout << "[warn] state apply failed\n";
            return false;
        }

        print_block_pretty(chain_.back(), chain_.size() - 1);
        return true;
    }

    // jei per si cikla nieko neiskaseme, grazinam false, bet kitame cikle vel bandys
    return false;
}
