#include "blockchain.h"
#include "hash_adapter.h"
#include "utils.h"
#include <algorithm>
#include <iostream>

Blockchain::Blockchain(std::string diff_prefix)
  : difficulty_(std::move(diff_prefix)) {
    //GENESIS
    Block genesis;
    genesis.header.prev_block_hash = std::string(64, '0');
    genesis.header.timestamp = now_ts();
    genesis.header.difficulty = difficulty_;
    genesis.header.txs_hash = ""; //nera tx
    genesis.header.nonce = 0;
    genesis.block_hash = hash_header(genesis.header); //suskaičiuoji hash’ą nuo header (be kasimo, nes genesis paprastai nekasamas)
    chain_.push_back(std::move(genesis)); //istatau i chaina
}

std::string Blockchain::calc_tx_id(const Transaction& t) const {
    //sudedu i viena stringa
    std::string payload = t.sender + "|" + t.receiver + "|" + std::to_string(t.amount) + "|" + std::to_string(t.nonce); 
    return HashAdapter::hash_string(payload); //pritaikau savo hash
}

//TRANSAKCIJU GENERAVIMAS
void Blockchain::init_transactions(size_t n_txs) {
    mempool_.clear(); //isvalom mempool (jei kazkas buvo anksciau)
    
    //sukuriam n_txs transakciju
    for (size_t i = 0; i < n_txs; ++i) {
        Transaction tx;

        // aprasti user siuntejas ir gavejas
        tx.sender   = "user_" + std::to_string(i % 100);
        tx.receiver = "user_" + std::to_string((i+1) % 100);
        
        //pinigu kiekis 1–100
        tx.amount = rand_u64(1, 100);
        //nonce tiesiog eiles numeris (kad kiekviena butu unikali)
        tx.nonce = i;
        //sukuriam hash kaip transakcijos ID
        std::string text = tx.sender + "|" + tx.receiver + "|" +std::to_string(tx.amount) + "|" +std::to_string(tx.nonce);
        tx.tx_id = HashAdapter::hash_string(text);

        mempool_.push_back(tx); //idedam i mempool sarasa
    }
    std::cout << "[mempool] Sukurta " << mempool_.size() << " transakcijų\n";
}

std::string Blockchain::calc_txs_hash(const std::vector<Transaction>& txs) const {
    //sujungiu visų tx_id į vieną ilgą stringą, tada per HashAdapter
    std::string concat; concat.reserve(txs.size() * 64);
    for (auto& t : txs) concat += t.tx_id;
    return HashAdapter::hash_string(concat);
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
    //Patikrina, ar hash prasideda tavo sunkumo prefiksu (pvz., "000")
    return hex.rfind(difficulty_, 0) == 0; 
    //PoW TAISYKLE
}

//KASIMAS PoW
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

        //kas tam tikra kieki bandymu parodom progresa
        if ((++iters & 0x3FFFF) == 0) {
            std::cout << "[mining] nonce=" << h.nonce << " hash=" << bh.substr(0,16) << "...\r" << std::flush;
        }
    } while (!valid_pow(bh)); //kol hash neprasideda pvz. "000"

    std::cout << "\n[mined] nonce=" << h.nonce << " hash=" << bh.substr(0,16) << "...\n";

    //4) sukuriam nauja bloka ir idedam i grandine
    Block b; b.header = h; b.txs = std::move(batch); b.block_hash = bh;
    chain_.push_back(std::move(b));

    return true;
}
