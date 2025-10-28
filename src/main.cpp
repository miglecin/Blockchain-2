#include "blockchain.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv) {
    //(jei vartotojas nieko neiveda)
    size_t n_txs   = 10000;    //transakciju kiekis
    size_t blocksz = 100;      //kiek transakciju viename bloke
    std::string diff = "000";  //Sunkumas (PoW tikslas)

    //pakeiciam numatytas reiksmes
    // ./blockchain [n_txs] [block_size] [difficulty]

    if (argc >= 3) n_txs   = std::strtoull(argv[2], nullptr, 10);
    if (argc >= 4) blocksz = std::strtoull(argv[3], nullptr, 10);
    if (argc >= 5) diff    = argv[4];

    //isvedam su kokiais parametrais programa dirbs
    std::cout << "  Txs=" << n_txs
              << "  BlockSize=" << blocksz
              << "  Diff=\"" << diff << "\"\n";

    //sukuriam blockchain su pasirinktu sunkumu
    Blockchain bc(diff);

    //sugeneruojam transakcijas ir uzpildom mempool
    std::cout << "[start] generuoju transakcijas...\n";
    bc.init_transactions(n_txs);

    //skaiciuosim kiek bloku pavyko iskasti
    size_t blocks_mined = 0;

    std::cout << "[start] pradedu kasima (Proof-of-Work)...\n";

    //kol dar yra transakciju mempoole, kasam blokus
    while (bc.mine_next_block(blocksz)) {
        ++blocks_mined;

        //grandines bukle po kiekvieno naujo bloko
        std::cout << "[chain] aukstis=" << bc.chain().size()
                  << "  liko_transakciju=" << bc.mempool().size() << "\n";
    }

    //kai transakciju neliko ir daugiau nebera ka daryti
    std::cout << "[done] Baigta. Iskasta bloku: " << blocks_mined << "\n";
    return 0;
}