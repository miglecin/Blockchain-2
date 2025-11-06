#include "blockchain.h"
#include "hash_adapter.h"
#include "utils.h"
#include <algorithm>
#include <iostream>

// ******************************************************
// *  PAGRINDINĖ BLOCKCHAIN BŪSENA (UTXO + users + tx)  *
// *  cia vyksta:                                       *
// *   - vartotojų kurimas                              *
// *   - pradinis UTXO generavimas                      *
// *   - transakcijų kurimas į mempool (UTXO stilius)   *
// *   - UTXO tikrinimas ir taikymas                    *
// *   - balanso skaičiavimas ir paieška                *
// ******************************************************

// bazinis bloko atlygis (50)
static const uint64_t BASE_BLOCK_REWARD = 50;

// paprasta halving taisykle kas 50 bloku
static uint64_t current_block_reward(size_t height) {
    size_t era = height / 50;
    uint64_t reward = BASE_BLOCK_REWARD >> era;
    if (reward == 0) reward = 1; //kad atlygis niekada netaptų nulis
    return reward;
}

// KONSTRUKTORIUS: sukuriame GENESIS bloką
Blockchain::Blockchain(std::string diff_prefix)
  : difficulty_(std::move(diff_prefix)) {
    //GENESIS (nekasamas)
    Block genesis;
    genesis.header.prev_block_hash = std::string(64, '0');
    genesis.header.timestamp = now_ts();
    genesis.header.difficulty = difficulty_;
    genesis.header.txs_hash = "";
    genesis.header.nonce = 0;
    genesis.block_hash = hash_header(genesis.header);
    chain_.push_back(std::move(genesis));
    std::cout << "[chain] genesis block created (height=0)\n";
}

// -----------------------------------------------------------
// Vartotoju generavimas ir pradinis UTXO
// -----------------------------------------------------------
void Blockchain::init_users(size_t n_users) {
    users_.clear();
    utxo_set_.clear();
    balances_.clear();

    users_.reserve(n_users);
    for (size_t i = 0; i < n_users; ++i) {
        User u;
        //sukuriam vartotojo vardą, jo "pubkey" = hash(name)
        u.name   = "user_" + std::to_string(i);
        u.pubkey = HashAdapter::hash_string(u.name);
        u.balance = rand_u64(100, 1'000'000); //duodam startinį balansą
        users_.push_back(u);

        //pradini balansa paverciame viena UTXO moneta
        Outpoint op{ "GENESIS_" + std::to_string(i), 0 }; //GENESIS_i:0 = netikras ankstesnis TX ID
        utxo_set_[op] = TxOut{ u.pubkey, u.balance };
    }
    recompute_balances();
    std::cout << "[users] generated " << users_.size() << " users and initial UTXO\n";
}

// -----------------------------------------------------------
// TRANSAKCIJOS: TX ID = hash nuo VIN/VOUT
// -----------------------------------------------------------
static std::string serialize_tx_for_id(const Transaction& t) {
    std::string s = "TX|VIN:";
    //input'ai (is kokiu monetu mokame)
    for (const auto& in : t.vin) {
        s += in.prev.txid + ":" + std::to_string(in.prev.index) + "|";
    }
    s += "|VOUT:";
    //output'ai (kam ir kiek siunciame)
    for (const auto& out : t.vout) {
        s += out.owner + ":" + std::to_string(out.value) + "|";
    }
    return s;
}
std::string Blockchain::calc_tx_id(const Transaction& t) const {
    return HashAdapter::hash_string(serialize_tx_for_id(t));
}

// -----------------------------------------------------------
// MEMPOOL TRANSAKCIJŲ GENERAVIMAS UTXO METODU
// -----------------------------------------------------------
void Blockchain::init_transactions(size_t n_txs) {
    mempool_.clear();
    //jeigu nera user'u - sukuriam
    if (users_.empty() || utxo_set_.empty()) {
        std::cout << "[warn] users/utxo empty, calling init_users(1000)\n";
        init_users(1000);
    }

    auto tmp_utxo = utxo_set_; //laikinas UTXO (naudojam tik generavimui)
    //sąrasas UTXO pasirinkimui
    std::vector<Outpoint> pool;
    pool.reserve(tmp_utxo.size());
    for (const auto& kv : tmp_utxo) pool.push_back(kv.first);

    if (pool.empty()) {
        std::cout << "[mempool] no UTXO to spend\n";
        return;
    }

    size_t pick_idx = 0, created = 0;
    //generuojame n transakcijų
    for (size_t i = 0; i < n_txs; ++i) {
        if (tmp_utxo.empty()) break;
        //jei baigėsi “pool”, atnaujiname ji
        if (pick_idx >= pool.size()) {
            pool.clear(); pool.reserve(tmp_utxo.size());
            for (const auto& kv : tmp_utxo) pool.push_back(kv.first);
            if (pool.empty()) break;
            pick_idx = 0;
        }
        //pasirenkam moneta (Outpoint)
        Outpoint src = pool[pick_idx++];
        auto it = tmp_utxo.find(src);
        if (it == tmp_utxo.end()) { --i; continue; } //jau panaudota

        const TxOut coin = it->second;
        //jeigu moneta labai maža → praleidžiam
        if (coin.value < 2) { tmp_utxo.erase(it); --i; continue; }

        // gavėjas = kitas user
        size_t ridx = (i + 1) % users_.size();
        std::string receiver = users_[ridx].pubkey;

        // siunčiame 1–100 (iki max-1)
        uint64_t send   = std::min<uint64_t>(rand_u64(1, 100), coin.value - 1);
        uint64_t change = coin.value - send;

        // formuojam transakciją
        Transaction tx;
        tx.vin.push_back(TxIn{ src });
        tx.vout.push_back(TxOut{ receiver, send });
        if (change > 0)
            tx.vout.push_back(TxOut{ coin.owner, change }); // grąža

        tx.tx_id = calc_tx_id(tx);

        // panaikinam seną ir įdedam naujus UTXO
        tmp_utxo.erase(src);
        for (size_t oi = 0; oi < tx.vout.size(); ++oi) {
            tmp_utxo[{tx.tx_id, (uint32_t)oi}] = tx.vout[oi];
            pool.push_back({tx.tx_id, (uint32_t)oi});
        }

        mempool_.push_back(std::move(tx));

        if ((++created % 1000) == 0)
            std::cout << "[mempool] generated " << created << " / " << n_txs << " txs (UTXO)\n";
    }

    std::cout << "[mempool] total " << mempool_.size() << " transactions ready (UTXO)\n";
}

// -----------------------------------------------------------
// UTXO Verifikacija bloke ir busenos pritaikymas
// -----------------------------------------------------------
bool Blockchain::verify_block_txs(const std::vector<Transaction>& txs) const {
    auto tmp = utxo_set_; //laikina kopija patikrai

    for (size_t ti = 0; ti < txs.size(); ++ti) {
        const auto& tx = txs[ti];

        // coinbase (pirma tx) TX0=coinbase
        if (ti == 0) {
            if (!tx.vin.empty()) return false; //coinbase neturi inputu
            if (tx.vout.empty()) return false; //privalo turet bent 1 isejima
            continue;
        }

        uint64_t sum_in = 0, sum_out = 0;

        //tikrinam ar input'ai egzistuoja
        for (const auto& in : tx.vin) {
            auto it = tmp.find(in.prev);
            if (it == tmp.end()) return false; //UTXO neegzistuoja (bando išleisti, ko neturi)
            sum_in += it->second.value;
        }

        //output sumos tikrinimas
        for (const auto& out : tx.vout) sum_out += out.value;
        if (sum_in < sum_out) return false; //Bandymas išleisti daugiau nei yra (inflacija)
        
        //tikrinimas nuo dvigubo isleidimo
        for (const auto& in : tx.vin) tmp.erase(in.prev); //pasalina panaudotus utxo
        
        //nauju utxo kurimas
        for (size_t oi = 0; oi < tx.vout.size(); ++oi) {
            Outpoint op{ tx.tx_id, static_cast<uint32_t>(oi) };
            tmp[op] = tx.vout[oi];
        }
        //tikrinam, kad TX ID atitinka VIN/VOUT hash (negalima slapta pakeisti TX)
        if (tx.tx_id != calc_tx_id(tx)) return false;
    }
    return true;
}
// ------------------------------------------------------------
// TAIKOME UTXO PO BLOKO
// ------------------------------------------------------------
bool Blockchain::apply_block_state(const Block& b) {
    for (size_t ti = 0; ti < b.txs.size(); ++ti) {
        const auto& tx = b.txs[ti];

        if (ti == 0) { // coinbase kuria UTXO
            for (size_t oi = 0; oi < tx.vout.size(); ++oi) {
                Outpoint op{ tx.tx_id, static_cast<uint32_t>(oi) };
                utxo_set_[op] = tx.vout[oi];
            }
            continue;
        }
        //pasalinam input'us
        for (const auto& in : tx.vin) {
            auto it = utxo_set_.find(in.prev);
            if (it == utxo_set_.end()) return false;
            utxo_set_.erase(it);
        }
        //pridedam naujus UTXO
        for (size_t oi = 0; oi < tx.vout.size(); ++oi) {
            Outpoint op{ tx.tx_id, static_cast<uint32_t>(oi) };
            utxo_set_[op] = tx.vout[oi];
        }
    }
    recompute_balances();
    return true;
}
// ------------------------------------------------------------
// PERSKAICIUOJAM BALANSUS iš UTXO
// ------------------------------------------------------------
void Blockchain::recompute_balances() {
    balances_.clear();
    for (const auto& kv : utxo_set_) {
        const TxOut& o = kv.second;
        balances_[o.owner] += o.value;
    }
}
// ------------------------------------------------------------
// PAIESKA GRANDINĖJE
// ------------------------------------------------------------
const Block* Blockchain::get_block_by_height(size_t height) const {
    if (height >= chain_.size()) return nullptr;
    return &chain_[height];
}

const Transaction* Blockchain::get_transaction_by_id(const std::string& tx_id) const {
    //ieskom blokuose
    for (const auto& b : chain_) {
        for (const auto& tx : b.txs) if (tx.tx_id == tx_id) return &tx;
    }
    //ieskom mempool'e
    for (const auto& tx : mempool_) if (tx.tx_id == tx_id) return &tx;
    return nullptr;
}
// ------------------------------------------------------------
// GRAŽUS TX SPAUSDINIMAS
// ------------------------------------------------------------
void Blockchain::print_transaction(const Transaction& tx) const {
    std::cout << "tx_id: " << tx.tx_id << "\n";
    for (size_t i = 0; i < tx.vin.size(); ++i) {
        const auto& in = tx.vin[i];
        std::cout << "  in[" << i << "]: " << in.prev.txid.substr(0,16) << ":" << in.prev.index << "\n";
    }
    for (size_t o = 0; o < tx.vout.size(); ++o) {
        const auto& out = tx.vout[o];
        std::cout << "  out[" << o << "]: to=" << out.owner.substr(0,16) << "... val=" << out.value << "\n";
    }
}
// ------------------------------------------------------------
// BALANSAS PAGAL PUBKEY
// ------------------------------------------------------------
bool Blockchain::get_balance(const std::string& owner, uint64_t& out) const {
    auto it = balances_.find(owner);
    if (it == balances_.end()) return false;
    out = it->second;
    return true;
}
// ------------------------------------------------------------
// VARTOTOJO SPAUSDINIMAS
// ------------------------------------------------------------
void Blockchain::print_user(size_t idx) const {
    if (idx >= users_.size()) { std::cout << "[user] index out of range\n"; return; }
    const auto& u = users_[idx];
    uint64_t bal = 0;
    if (!get_balance(u.pubkey, bal)) {
        std::cout << "[user] " << u.name << " (no balance entry)\n"; return;
    }
    std::cout << "user:    " << u.name << "\n";
    std::cout << "pubkey:  " << u.pubkey.substr(0, 32) << "...\n";
    std::cout << "balance: " << bal << "\n";
}
