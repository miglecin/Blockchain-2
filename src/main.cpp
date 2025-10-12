#include "blockchain.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv) {
    //(jei vartotojas nieko neiveda)
    size_t n_users = 1000;     //vartotoju kiekis
    size_t n_txs   = 10000;    //transakciju kiekis
    size_t blocksz = 100;      //kiek transakciju viename bloke
    std::string diff = "000";  //Sunkumas (PoW tikslas)

    //pakeiciam numatytas reiksmes
    if (argc >= 2) n_users = std::strtoull(argv[1], nullptr, 10);
    if (argc >= 3) n_txs   = std::strtoull(argv[2], nullptr, 10);
    if (argc >= 4) blocksz = std::strtoull(argv[3], nullptr, 10);
    if (argc >= 5) diff    = argv[4];

    //isvedam su kokiais parametrais programa dirbs
    std::cout << "Users=" << n_users
              << "  Txs=" << n_txs
              << "  BlockSize=" << blocksz
              << "  Diff=\"" << diff << "\"\n";

    //sukuriam blockchain su pasirinktu sunkumu
    Blockchain bc(diff);

    // inicializuojam transakcijas
    bc.init_transactions(n_txs);

    //kintamasis, kuris saugo, kiek bloku iskasem
    size_t blocks_mined = 0;

    //kol dar yra transakciju mempoole - kasam blokus
    while (bc.mine_next_block(blocksz)) {
        ++blocks_mined;
        std::cout << "[chain] aukstis=" << bc.chain().size()
                  << "  liko_transakciju=" << bc.mempool().size() << "\n";
    }

    //kai baigiam kasti
    std::cout << "Baigta. Iskasta bloku: " << blocks_mined << "\n";
    return 0;
}
