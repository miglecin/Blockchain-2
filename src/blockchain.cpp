#include "blockchain.h"
#include "hash_adapter.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <chrono>

// block explorer spausdinimas (adaptuotas UTXO)
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
    size_t preview_count = std::min((size_t)5, b.txs.size());
    for (size_t i = 0; i < preview_count; i++) {
        const Transaction& tx = b.txs[i];
        std::cout << " tx[" << i << "]: id=" << tx.tx_id.substr(0,16) << "...\n";
        for (size_t j = 0; j < tx.vin.size(); ++j) {
            const TxIn& in = tx.vin[j];
            std::cout << "   in[" << j << "]: " << in.prev.txid.substr(0,16) << ":" << in.prev.index << "\n";
        }
        for (size_t k = 0; k < tx.vout.size(); ++k) {
            const TxOut& out = tx.vout[k];
            std::cout << "   out[" << k << "]: to=" << out.owner.substr(0,16) << "... val=" << out.value << "\n";
        }
    }
    if (b.txs.size() > preview_count) {
        std::cout << " ... (" << (b.txs.size() - preview_count) << " more txs not shown)\n";
    }
    std::cout << "========================================\n\n";
}

// bazinis bloko atlygis (50)
static const uint64_t BASE_BLOCK_REWARD = 50;

// paprasta halving taisykle kas 50 bloku
static uint64_t current_block_reward(size_t height) {
    size_t era = height / 50;
    uint64_t reward = BASE_BLOCK_REWARD >> era;
    if (reward == 0) reward = 1;
    return reward;
}

Blockchain::Blockchain(std::string diff_prefix)
  : difficulty_(std::move(diff_prefix)) {
    // GENESIS (nekasamas)
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

void Blockchain::init_users(size_t n_users) {
    //isvalom buvusius vartotojus ir UTXO rinkini
    users_.clear();
    utxo_set_.clear();
    balances_.clear();

    //rezervuojam vietos (optimizacija)
    users_.reserve(n_users);
    for (size_t i = 0; i < n_users; ++i) {
        User u;
        // ----- vartotojo duomenys -----
        u.name   = "user_" + std::to_string(i);
        u.pubkey = HashAdapter::hash_string(u.name); //viesasis raktas = hash nuo vardo
        u.balance = rand_u64(100, 1'000'000); // duodam atsitiktini startini balansa
        users_.push_back(u); //isaugom vartotoja

        // ----- pradiniai pinigai kaip UTXO -----
        // cia sukuriam "voka su pinigais" (UTXO)
        Outpoint op{ "GENESIS_" + std::to_string(i), 0 }; // GENESIS_i:0 -- padirbtas pradinis outpoint
        utxo_set_[op] = TxOut{ u.pubkey, u.balance }; // i UTXO irasom: savininka (pubkey) ir verte (balance)
    }
    recompute_balances(); // perskaiciuojam balansas_ pagal UTXO rinkini
    std::cout << "[users] generated " << users_.size() << " users and initial UTXO\n";
}
// -----------------------------------------------------------
// TX ID generavimas (kaip Bitcoin)
// ID priklauso nuo VIN/VOUT turinio
// tai uztikrina, kad dvi skirtingos transakcijos niekada
// netures vienodo ID
// -----------------------------------------------------------
static std::string serialize_tx_for_id(const Transaction& t) {
    std::string s = "TX|VIN:";

    // VIN (is kokiu UTXO imam pinigus)
    for (const auto& in : t.vin) {
        // pridedam ankstesnes monetos (UTXO) nuoroda
        s += in.prev.txid + ":" + std::to_string(in.prev.index) + "|";
    }
    s += "|VOUT:";
    //VOUT (kam ir kiek nauju monetu sukuriam)
    for (const auto& out : t.vout) {
        s += out.owner + ":" + std::to_string(out.value) + "|";
    }
    return s;
}
// paverciame visa TX turini i hash (tai = TX ID)
std::string Blockchain::calc_tx_id(const Transaction& t) const {
    return HashAdapter::hash_string(serialize_tx_for_id(t)); // Hash nuo VIN/VOUT aprasymo
}

// TRANSAKCIJU GENERAVIMAS (UTXO) – be konfliktu, su laikinu UTXO vaizdu
void Blockchain::init_transactions(size_t n_txs) {
    mempool_.clear(); // isvalom mempool (pradedam nuo tuscio saraso)

    // jeigu dar nera naudotoju ar UTXO rinkinio – sugeneruojam
    if (users_.empty() || utxo_set_.empty()) {
        std::cout << "[warn] users/utxo empty, calling init_users(1000)\n";
        init_users(1000);
    }

    // pasidarom LAIKINA UTXO kopija (tmp_utxo), kad galetume kurti
    // nuoseklia transakciju grandine nesugadinant tikros busenos
    auto tmp_utxo = utxo_set_;

    // "pool" – sarasas is visų esamu UTXO raktu (Outpoint), kad butu patogu rinktis
    std::vector<Outpoint> pool;
    pool.reserve(tmp_utxo.size());
    for (const auto& kv : tmp_utxo) pool.push_back(kv.first);

    // jeigu neturim ko leisti – baigiam
    if (pool.empty()) {
        std::cout << "[mempool] no UTXO to spend\n";
        return;
    }

    size_t pick_idx = 0; //i kuri pool elementa ziurim (ziedinis perejimas)
    size_t created = 0; //kiek transakciju jau sukurta (progress log)

    //kuriam iki n_txs transakciju (arba kol baigsis UTXO)
    for (size_t i = 0; i < n_txs; ++i) {
        if (tmp_utxo.empty()) break;

        // jeigu pasiekem pool pabaiga – atnaujinam ji is dabartinio tmp_utxo
        if (pick_idx >= pool.size()) {
            pool.clear(); pool.reserve(tmp_utxo.size());
            for (const auto& kv : tmp_utxo) pool.push_back(kv.first);
            if (pool.empty()) break; // visai nebera UTXO
            pick_idx = 0;
        }

        Outpoint src = pool[pick_idx++]; //paimam kandidata is pool
        auto it = tmp_utxo.find(src); //patikrinam ar sis UTXO dar yra nenaudotas (tmp_utxo)
        if (it == tmp_utxo.end()) { --i; continue; }  // jis jau buvo sunaudotas anksciau sios generacijos metu. mazinam i, kad bendrai vis tiek pabandytume sukurti n_txs vnt.

        const TxOut coin = it->second; //UTXO verte ir savininkas
            // jeigu verte < 2, nelabai yra vietos grazai – praleidziam sita UTXO
        if (coin.value < 2) { tmp_utxo.erase(it); --i; continue; }  // ismetam is laikino rinkinio ir bandom kita

        //parenkam gaveja is users (paprastai i+1)
        size_t ridx = (i + 1) % users_.size();
        std::string receiver = users_[ridx].pubkey;

        //kiek siusti gavejui (1..min(100, coin.value-1)), kad liktu grazai
        uint64_t send = std::min<uint64_t>(rand_u64(1, 100), coin.value - 1);
        uint64_t change = coin.value - send;


        // suformuojam UTXO transakcija:
        //  - vienas input (sunaudoja src)
        //  - vienas ar du output'ai (gavėjui + graza atgal savininkui)
        Transaction tx;
        tx.vin.push_back(TxIn{ src });
        tx.vout.push_back(TxOut{ receiver, send });
        if (change > 0) tx.vout.push_back(TxOut{ coin.owner, change }); // graza atgal tam paciam savininkui, kuris turejo src
        tx.tx_id = calc_tx_id(tx); //paskaičiuojam deterministini tx_id is VIN/VOUT turinio

        //ATNAUJINAM LAIKINA UTXO BUSENA:
        // 1) panaudota sena isejima (src) pasalinam
        tmp_utxo.erase(src);

        // 2) visi nauji isejimai tampa NAUJAIS UTXO (kad kitos tx galetu juos naudoti)
        for (size_t oi = 0; oi < tx.vout.size(); ++oi) {
            Outpoint op{ tx.tx_id, static_cast<uint32_t>(oi) };
            tmp_utxo[op] = tx.vout[oi];
            pool.push_back(op); // itraukiam i pool, kad veliau galetume pasirinkti ir siuos
        }

        mempool_.push_back(std::move(tx));// paties tx idedam i MEMPOOL ta pacia tvarka
        //kas 1000 – progreso eilute
        if ((++created % 1000) == 0) {
            std::cout << "[mempool] generated " << created << " / " << n_txs << " txs (UTXO)\n";
        }
    }

    std::cout << "[mempool] total " << mempool_.size() << " transactions ready (UTXO)\n";
}

std::string Blockchain::merkle_root(std::vector<std::string> ids) const {
    if (ids.empty()) return HashAdapter::hash_string("");
    while (ids.size() > 1) {
        if (ids.size() & 1) ids.push_back(ids.back());
        std::vector<std::string> next;
        next.reserve(ids.size() / 2);
        for (size_t i = 0; i < ids.size(); i += 2) {
            next.push_back(HashAdapter::hash_string(ids[i] + ids[i + 1]));
        }
        ids.swap(next);
    }
    return ids[0];
}

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

std::string Blockchain::hash_header(const BlockHeader& h) const {
    return HashAdapter::hash_string(serialize_header(h));
}

bool Blockchain::valid_pow(const std::string& hex) const {
    return hex.rfind(difficulty_, 0) == 0;
}

const Block* Blockchain::get_block_by_height(size_t height) const {
    if (height >= chain_.size()) return nullptr;
    return &chain_[height];
}

const Transaction* Blockchain::get_transaction_by_id(const std::string& tx_id) const {
    for (const auto& b : chain_) {
        for (const auto& tx : b.txs) if (tx.tx_id == tx_id) return &tx;
    }
    for (const auto& tx : mempool_) if (tx.tx_id == tx_id) return &tx;
    return nullptr;
}

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

void Blockchain::print_block(const Block& b, size_t height) const {
    print_block_pretty(b, height);
}

// verifikuoja UTXO taisykles (paprastas modelis)
bool Blockchain::verify_block_txs(const std::vector<Transaction>& txs) const {
    auto tmp = utxo_set_; // laikinas UTXO rinkinys verifikacijai (kopija nuo dabartines busenos)

    for (size_t ti = 0; ti < txs.size(); ++ti) {
        const auto& tx = txs[ti];

        // ---- COINBASE (pirma tx bloke) ----
        // coinbase neturi VIN ir privalo tureti bent viena VOUT
        if (ti == 0) {
            if (!tx.vin.empty()) return false; // coinbase negali tureti input
            if (tx.vout.empty()) return false; // coinbase privalo kurti isejima
            continue; // pereinam prie paprastu tx
        }
        // ---- PAPRASTA TX ----
        uint64_t sum_in = 0, sum_out = 0;

        // 1) patikrinam, kad visi input UTXO egzistuoja laikiname rinkinyje ir suskaiciuojam bendra input suma
        for (const auto& in : tx.vin) {
            auto it = tmp.find(in.prev);
            if (it == tmp.end()) return false; // nera tokio UTXO -> neteisinga
            sum_in += it->second.value;
        }
        // 2) suskaiciuojam output suma
        for (const auto& out : tx.vout) sum_out += out.value;
        // 3) negalima isleisti daugiau nei turi
        if (sum_in < sum_out) return false;
        // 4) panaudotus input UTXO ismetam (kad butu neimanomas dvigubas isleidimas)
        for (const auto& in : tx.vin) tmp.erase(in.prev);
        // 5) is VOUT suformuojam naujus UTXO i ta pati laikina rinkinuka
        for (size_t oi = 0; oi < tx.vout.size(); ++oi) {
            Outpoint op{ tx.tx_id, static_cast<uint32_t>(oi) };
            tmp[op] = tx.vout[oi];
        }
        // 6) vientisumas: tx_id turi buti lygiai is VIN/VOUT turinio
        if (tx.tx_id != calc_tx_id(tx)) return false;
    }

    return true;
}

// pritaiko UTXO busena ir perskaiciuoja balances_ | Svarbu: kvieciama TIK po sekmingo verify_block_txs.
bool Blockchain::apply_block_state(const Block& b) {
    for (size_t ti = 0; ti < b.txs.size(); ++ti) {
        const auto& tx = b.txs[ti];

        // ---- COINBASE ----
        if (ti == 0) {
            // coinbase neturi input, tik sukuria naujus UTXO is VOUT
            for (size_t oi = 0; oi < tx.vout.size(); ++oi) {
                Outpoint op{ tx.tx_id, static_cast<uint32_t>(oi) };
                utxo_set_[op] = tx.vout[oi];
            }
            continue;
        }

        // ---- PAPRASTA TX ----
        // 1) panaikinam panaudotus input UTXO is tikro rinkinio
        for (const auto& in : tx.vin) {
            auto it = utxo_set_.find(in.prev);
            if (it == utxo_set_.end()) return false; // neturetu nutikti po verify
            utxo_set_.erase(it);
        }
        // 2) pridedam visus naujus VOUT kaip UTXO
        for (size_t oi = 0; oi < tx.vout.size(); ++oi) {
            Outpoint op{ tx.tx_id, static_cast<uint32_t>(oi) };
            utxo_set_[op] = tx.vout[oi];
        }
    }
    // perskaiciuojam pagal nauja UTXO busena
    recompute_balances();
    return true;
}
//perskaiciuoja balances_ lentele is UTXO rinkinio
void Blockchain::recompute_balances() {
    balances_.clear();
    for (const auto& kv : utxo_set_) {
        const TxOut& o = kv.second;
        balances_[o.owner] += o.value;
    }
}
//Suformuoja bloko kandidata is MEMPOOL pradzios
Candidate Blockchain::build_candidate_from_front(size_t block_size) const {
    Candidate c;

    // ----- 1) COINBASE (tx[0]) -----
    // Atlygis priklauso nuo aukscio (paprastas halving kas 50 bloku).
    // Coinbase turi tuscia VIN ir bent viena VOUT. Cia vienas VOUT -> "miner_0".
    uint64_t reward = current_block_reward(chain_.size());
    Transaction coinbase;
    coinbase.vout.push_back(TxOut{ "miner_0", reward });
    coinbase.tx_id = calc_tx_id(coinbase); // TX ID nuo VIN/VOUT turinio (VIN tuscias, vienas VOUT -> miner_0, reward).
    c.txs.push_back(std::move(coinbase));// Dedame coinbase i tx[0]

     // ----- 2) MEMPOOL SNAPSHOT -----
    // Paimame dar (block_size - 1) transakciju is mempool PRADZIOS.
    // Svarbu: cia kopijuojame duomenis (snapshot). Tik kai blokas bus
    // sekmingai iskastas ir priimtas, tada tikrai "isnaudosime" tiek
    // mempool'o elementu (pop_front) kitur (mine_next_block*).
    size_t taken = 0;
    for (const auto& tx : mempool_) {
        if (taken >= block_size - 1) break;
        c.txs.push_back(tx);
        ++taken;
    }

    // ----- 3) HEADER (be nonce) -----
    // prev_block_hash: rodo i paskutinio bloko hash grandineje
    c.header.prev_block_hash = chain_.back().block_hash;
    c.header.timestamp       = now_ts();
    c.header.difficulty      = difficulty_;
    std::vector<std::string> ids; ids.reserve(c.txs.size());// merkle root
    for (auto& t : c.txs) ids.push_back(t.tx_id);
    c.header.txs_hash        = merkle_root(std::move(ids));
    c.header.nonce           = 0; //nonce pradzioje 0 (kasimo ciklas ji didins)

    return c;
}

bool Blockchain::try_mine_header(BlockHeader& h, std::string& out_hash, uint64_t max_ms) const {
    using clk = std::chrono::steady_clock;
    auto start = clk::now();
    size_t iters = 0;
    while (true) {
        ++h.nonce;
        out_hash = hash_header(h);
        if (valid_pow(out_hash)) return true;
        if ((++iters & 0x3FFFF) == 0) {
            auto now = clk::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (ms >= (long long)max_ms) break;
            std::cout << "[mining] nonce=" << h.nonce
                      << " hash=" << out_hash.substr(0,16) << "...\r" << std::flush;
        }
    }
    return false;
}

bool Blockchain::mine_next_block(size_t block_size) {
    if (mempool_.empty()) return false;

    Candidate cand = build_candidate_from_front(block_size);

    if (!verify_block_txs(cand.txs)) {
        std::cout << "[block] verification failed\n";
        return false;
    }

    std::string bh;
    BlockHeader h = cand.header;
    size_t iters = 0;
    do {
        ++h.nonce;
        bh = hash_header(h);
        if ((++iters & 0x3FFFF) == 0) {
            std::cout << "[mining] nonce=" << h.nonce << " hash=" << bh.substr(0,16) << "...\r" << std::flush;
        }
    } while (!valid_pow(bh));

    std::cout << "\n[mined] block found! nonce=" << h.nonce << " hash=" << bh.substr(0,16) << "...\n";

    size_t need = cand.txs.size() - 1;
    for (size_t i = 0; i < need && !mempool_.empty(); ++i) mempool_.pop_front();

    Block b; b.header = h; b.txs = std::move(cand.txs); b.block_hash = bh;
    chain_.push_back(std::move(b));
    if (!apply_block_state(chain_.back())) {
        std::cout << "[warn] state apply failed\n";
        return false;
    }
    print_block_pretty(chain_.back(), chain_.size() - 1);
    return true;
}

bool Blockchain::mine_next_block_v2(size_t block_size, size_t num_candidates, uint64_t max_ms) {
    if (mempool_.empty()) return false;

    std::vector<Candidate> cands; cands.reserve(num_candidates);
    for (size_t i = 0; i < num_candidates; ++i) {
        cands.push_back(build_candidate_from_front(block_size));
        cands.back().header.timestamp += i; // maza variacija
    }

    for (auto it = cands.begin(); it != cands.end(); ) {
        if (verify_block_txs(it->txs)) ++it;
        else it = cands.erase(it);
    }
    if (cands.empty()) {
        std::cout << "[block] no valid candidates (verification failed)\n";
        return false;
    }

    for (auto& cand : cands) {
        std::string bh; BlockHeader h = cand.header;
        bool ok = try_mine_header(h, bh, max_ms);
        if (!ok) { std::cout << "[mining] candidate timed out (no solution)\n"; continue; }

        std::cout << "\n[mined] block found! nonce=" << h.nonce
                  << " hash=" << bh.substr(0,16) << "...\n";

        size_t need = cand.txs.size() - 1;
        for (size_t i = 0; i < need && !mempool_.empty(); ++i) mempool_.pop_front();

        Block b; b.header = h; b.txs = std::move(cand.txs); b.block_hash = bh;
        chain_.push_back(std::move(b));
        if (!apply_block_state(chain_.back())) {
            std::cout << "[warn] state apply failed\n";
            return false;
        }
        print_block_pretty(chain_.back(), chain_.size() - 1);
        return true;
    }
    return false;
}

bool Blockchain::get_balance(const std::string& owner, uint64_t& out) const {
    auto it = balances_.find(owner);
    if (it == balances_.end()) return false;
    out = it->second;
    return true;
}

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
