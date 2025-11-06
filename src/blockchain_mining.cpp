#include "blockchain.h"
#include "hash_adapter.h"
#include "utils.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

// -----------------------------------------------------------
//Merkle root: sujungiam visus tx hash i 1 "bloko pirsto atspauda"
//Jei nelyginis skaicius - dubliuojam paskutini ir t.t.
// -----------------------------------------------------------
std::string Blockchain::merkle_root(std::vector<std::string> ids) const {
    if (ids.empty()) return HashAdapter::hash_string("");
    while (ids.size() > 1) {
        if (ids.size() & 1) ids.push_back(ids.back());//jei nelyginis skaicius - dubliuojam
        std::vector<std::string> next;
        next.reserve(ids.size() / 2);
        for (size_t i = 0; i < ids.size(); i += 2) {
            // hash(hashA + hashB)
            next.push_back(HashAdapter::hash_string(ids[i] + ids[i + 1]));
        }
        ids.swap(next);
    }
    return ids[0]; // vienas galutinis hash
}
// -----------------------------------------------------------
//Bloko header pavertimas i teksta ir hash
// -----------------------------------------------------------
std::string Blockchain::serialize_header(const BlockHeader& h) const {
    std::string s;
    s += h.prev_block_hash;
    s += "|" + std::to_string(h.timestamp);
    s += "|" + h.version;
    s += "|" + h.txs_hash;
    s += "|" + std::to_string(h.nonce);
    s += "|" + h.difficulty;
    return s;
}
//hash nuo header teksto - blocko hashas
std::string Blockchain::hash_header(const BlockHeader& h) const {
    return HashAdapter::hash_string(serialize_header(h));
}
//ar prasideda nuo 000?
bool Blockchain::valid_pow(const std::string& hex) const {
    return hex.rfind(difficulty_, 0) == 0;
}
// ===========================================================
//  Fee helper’iai 
// ===========================================================
uint64_t Blockchain::calc_tx_fee(const Transaction& /*tx*/) const {
    // Paprasta politika: fiksuotas 1 už kiekvieną paprastą tx
   return TX_FEE > 0 ? TX_FEE : 1;
}
void Blockchain::add_fees_to_coinbase(Transaction& coinbase, const std::vector<Transaction>& txs) const {
    // coinbase yra tx[0], kitos tx kaupia mokesčius
    if (coinbase.vout.empty()) return;

    uint64_t total_fees = 0;
    // nuo tx[1] iki pabaigos
    for (size_t i = 1; i < txs.size(); ++i) {
        total_fees += calc_tx_fee(txs[i]);
    }

    // pridedam prie coinbase pirmo vout (miner_0)
    coinbase.vout[0].value += total_fees;

    // coinbase ID priklauso nuo VOUT turinio -> reikia perskaičiuoti
    coinbase.tx_id = calc_tx_id(coinbase);
}
// -----------------------------------------------------------
//Sukuriam bloko kandidata (FEES -> coinbase + tx is mempool pradzios)
// -----------------------------------------------------------
static uint64_t current_block_reward(size_t height) {
    const uint64_t BASE_BLOCK_REWARD = 50;
    size_t era = height / 50; 
    uint64_t reward = BASE_BLOCK_REWARD >> era;
    if (reward == 0) reward = 1;
    return reward;
}

Candidate Blockchain::build_candidate_from_front(size_t block_size) const {
    Candidate c;

    // 1) coinbase su baziniu atlygiu
    uint64_t reward = current_block_reward(chain_.size());
    Transaction coinbase;
    coinbase.vout.push_back(TxOut{ "miner_0", reward });
    coinbase.tx_id = calc_tx_id(coinbase);
    c.txs.push_back(std::move(coinbase));

    // 2) pridėti dar (block_size - 1) tx iš mempool (snapshot)
    size_t taken = 0;
    for (const auto& tx : mempool_) {
        if (taken >= block_size - 1) break;
        c.txs.push_back(tx);
        ++taken;
    }

    // 3) PRIEŠ merkle — pridedam mokesčius prie coinbase
    add_fees_to_coinbase(c.txs[0], c.txs);

    // 4) header (be nonce)
    c.header.prev_block_hash = chain_.back().block_hash;
    c.header.timestamp       = now_ts();
    c.header.difficulty      = difficulty_;
    std::vector<std::string> ids; ids.reserve(c.txs.size());
    for (auto& t : c.txs) ids.push_back(t.tx_id);
    c.header.txs_hash        = merkle_root(std::move(ids));
    c.header.nonce           = 0;

    return c;
}

// -----------------------------------------------------------
//kasyba su laiko limitu (v0.2), bandom kol randam gera hash ARBA baigaisi laikas
// -----------------------------------------------------------
bool Blockchain::try_mine_header(BlockHeader& h, std::string& out_hash, uint64_t max_ms) const {
    using clk = std::chrono::steady_clock;
    auto start = clk::now();
    size_t iters = 0;
    while (true) {
        ++h.nonce; //didinam nonce
        out_hash = hash_header(h); //skaiciuojam hash
        if (valid_pow(out_hash)) return true; //jei hash prasideda su 000
       //kas N interaciju tikrinam ar nesibaige laikas
        if ((++iters & 0x3FFFF) == 0) {
            auto now = clk::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (ms >= (long long)max_ms) break; //laikas baigesi

            std::cout << "[mining] nonce=" << h.nonce
                      << " hash=" << out_hash.substr(0,16) << "...\r" << std::flush;
        }
    }
    return false; //nepavyko per laiko limita
}

// -----------------------------------------------------------
// Kasyba v0.1 (vienas kandidatas, be laiko limito), tol kol randam nonce kuris ataitinka diff
// -----------------------------------------------------------
bool Blockchain::mine_next_block(size_t block_size) {
    if (mempool_.empty()) return false;

    Candidate cand = build_candidate_from_front(block_size);
    
    //tikrinam transakcijas pagal UTXO taisykles
    if (!verify_block_txs(cand.txs)) {
        std::cout << "[block] verification failed\n";
        return false;
    }

    std::string bh;
    BlockHeader h = cand.header;
    size_t iters = 0;
    // brute force nonce: sukame kol hash tinka difficulty
    do {
        ++h.nonce;
        bh = hash_header(h);
        if ((++iters & 0x3FFFF) == 0) {
            std::cout << "[mining] nonce=" << h.nonce
                      << " hash=" << bh.substr(0,16) << "...\r" << std::flush;
        }
    } while (!valid_pow(bh));

    std::cout << "\n[mined] block found! nonce=" << h.nonce
              << " hash=" << bh.substr(0,16) << "...\n";

    //is mempool isimam transakcijas kurios pateko i bloka (be coinbase)
    size_t need = cand.txs.size() - 1;
    for (size_t i = 0; i < need && !mempool_.empty(); ++i) mempool_.pop_front();

    //dedam bloka i grandine
    Block b; b.header = h; b.txs = std::move(cand.txs); b.block_hash = bh;
    chain_.push_back(std::move(b));
    //atnaujinam UTXO busena
    if (!apply_block_state(chain_.back())) {
        std::cout << "[warn] state apply failed\n";
        return false;
    }
    
    print_block(chain_.back(), chain_.size() - 1);
    return true;
}

// -----------------------------------------------------------
// Kasyba v0.2
// Kuriam kelis bloku kandidatus, tikriname kuri pavyks iskasti
// kiekvienam suteikiam laiko limita (5ms)
// -----------------------------------------------------------
bool Blockchain::mine_next_block_v2(size_t block_size, size_t num_candidates, uint64_t max_ms) {
    if (mempool_.empty()) return false;
    //kuriam kelis kandidatus i bloka
    std::vector<Candidate> cands; cands.reserve(num_candidates);
    for (size_t i = 0; i < num_candidates; ++i) {
        cands.push_back(build_candidate_from_front(block_size));
        cands.back().header.timestamp += i; //maza variacija (kad skirtusi hash)
    }
    //isimetam blogus kandidatus (UTXO klaidos)
    for (auto it = cands.begin(); it != cands.end(); ) {
        if (verify_block_txs(it->txs)) ++it;
        else it = cands.erase(it);
    }
    if (cands.empty()) {
        std::cout << "[block] no valid candidates (verification failed)\n";
        return false;
    }
    //bandome kiekviena kandidata per max_ms laiko
    for (auto& cand : cands) {
        std::string bh; BlockHeader h = cand.header;
        bool ok = try_mine_header(h, bh, max_ms);
        if (!ok) { std::cout << "[mining] candidate timed out (no solution)\n"; continue; }

        std::cout << "\n[mined] block found! nonce=" << h.nonce
                  << " hash=" << bh.substr(0,16) << "...\n";

        //isimam panaudotas tx is mempool (be coinbase)
        size_t need = cand.txs.size() - 1;
        for (size_t i = 0; i < need && !mempool_.empty(); ++i) mempool_.pop_front();

        //dedam bloka i grandine
        Block b; b.header = h; b.txs = std::move(cand.txs); b.block_hash = bh;
        chain_.push_back(std::move(b));
        //atnaujinam UTXO
        if (!apply_block_state(chain_.back())) {
            std::cout << "[warn] state apply failed\n";
            return false;
        }
        print_block(chain_.back(), chain_.size() - 1);
        return true;
    }
    return false; //nei vieno nepavyko iskasti laiku
}

// -----------------------------------------------------------
// Kasyba v0.2 (lygiagreciai, tikras multithread)
// Kiekvienas kandidatas kasamas atskirame threade
// Pirmas radęs laimi, kiti nutraukiami
// -----------------------------------------------------------
bool Blockchain::mine_next_block_v2_parallel(size_t block_size, size_t num_candidates, uint64_t max_ms) {
    if (mempool_.empty()) return false;

    // 1) Sukuriam kandidatus (snapshot nuo mempool pradzios)
    std::vector<Candidate> cands; 
    cands.reserve(num_candidates);
    for (size_t i = 0; i < num_candidates; ++i) {
        cands.push_back(build_candidate_from_front(block_size));
        cands.back().header.timestamp += i; // maza variacija tarp headeriu
    }

    // 2) Ismetam blogus (UTXO klaidos)
    for (auto it = cands.begin(); it != cands.end(); ) {
        if (verify_block_txs(it->txs)) ++it;
        else it = cands.erase(it);
    }
    if (cands.empty()) {
        std::cout << "[block] no valid candidates (verification failed)\n";
        return false;
    }

    // 3) Lygiagretus kasimas
    std::atomic<bool> stop(false);     // true - visi kiti threadai baigia darba
    std::atomic<int>  winner(-1);      // laimejusio kandidato indeksas
    std::mutex        mtx;             // apsaugai kai irasom laimetojo duomenis

    // saugosime laimetojo header/hash
    BlockHeader winner_h{};
    std::string winner_bh;

    auto start = std::chrono::steady_clock::now();

    // darbine funkcija kiekvienam kandidatui
    auto worker = [&](size_t idx) {
        BlockHeader h = cands[idx].header;
        std::string bh;
        size_t iters = 0;

        while (!stop.load(std::memory_order_relaxed)) {
            ++h.nonce;
            bh = hash_header(h);
            if (valid_pow(bh)) {
                // pirmas kuris nustate stop=true laimi
                bool was_stopped = stop.exchange(true);
                if (!was_stopped) {
                    std::lock_guard<std::mutex> lk(mtx);
                    winner = static_cast<int>(idx);
                    winner_h = h;
                    winner_bh = bh;
                }
                return;
            }
            // kas ~260k tikrinam laika arba stop veliavele
            if ((++iters & 0x3FFFF) == 0) {
                if (stop.load(std::memory_order_relaxed)) return;
                auto now = std::chrono::steady_clock::now();
                auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                if (ms >= (long long)max_ms) return; // laikas baigesi siam threade
                std::cout << "[mining] cand=" << idx
                          << " nonce=" << h.nonce
                          << " hash="  << bh.substr(0,16) << "...\r" << std::flush;
            }
        }
    };

    // paleidziam po viena threada kiekvienam kandidatui (arba iki num_candidates)
    std::vector<std::thread> ths;
    ths.reserve(cands.size());
    for (size_t i = 0; i < cands.size(); ++i) {
        ths.emplace_back(worker, i);
    }
    // laukiam visu
    for (auto& t : ths) t.join();

    // 4) Ar turim laimetoja?
    if (winner.load() < 0) {
        std::cout << "[mining] no solution within time limit\n";
        return false;
    }

    int wi = winner.load();
    std::cout << "\n[mined] block found! cand=" << wi
              << " nonce=" << winner_h.nonce
              << " hash="  << winner_bh.substr(0,16) << "...\n";

    // 5) Ismempoolinam tiek, kiek sunaudojo laimetojo kandidatas (be coinbase)
    size_t need = cands[wi].txs.size() - 1;
    for (size_t i = 0; i < need && !mempool_.empty(); ++i) mempool_.pop_front();

    // 6) Suformuojam bloka, pritaikom UTXO busena
    Block b;
    b.header     = winner_h;
    b.txs        = std::move(cands[wi].txs);
    b.block_hash = winner_bh;

    chain_.push_back(std::move(b));
    if (!apply_block_state(chain_.back())) {
        std::cout << "[warn] state apply failed\n";
        return false;
    }

    print_block(chain_.back(), chain_.size() - 1);
    return true;
}
