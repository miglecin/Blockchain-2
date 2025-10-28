 // CLI usage:
    //   ./blockchain [n_txs] [block_size] [difficulty]
    //
    // examples:
    //   ./blockchain
    //   ./blockchain 5000
    //   ./blockchain 5000 50
    //   ./blockchain 5000 50 0000
#include "blockchain.h"
#include <iostream>
#include <sstream> //for interactive REPL parsing
#include <cstdlib>

int main(int argc, char** argv) {
    //(jei vartotojas nieko neiveda)
    size_t n_txs   = 10000;    //transakciju kiekis
    size_t blocksz = 100;      //kiek transakciju viename bloke
    std::string diff = "000";  //Sunkumas (PoW tikslas)

    //pakeiciam numatytas reiksmes
    if (argc >= 3) n_txs   = std::strtoull(argv[1], nullptr, 10);
    if (argc >= 4) blocksz = std::strtoull(argv[2], nullptr, 10);
    if (argc >= 5) diff    = argv[3];

    //isvedam su kokiais parametrais programa dirbs
    std::cout << "  Txs=" << n_txs
              << "  BlockSize=" << blocksz
              << "  Diff=\"" << diff << "\"\n";

    //sukuriam blockchain su pasirinktu sunkumu
    Blockchain bc(diff);

    //sugeneruojam transakcijas ir uzpildom mempool
    std::cout << "[start] generating transactions...\n";
    bc.init_transactions(n_txs);

    //skaiciuosim kiek bloku pavyko iskasti
    std::cout << "[start] begin mining (Proof-of-Work)...\n";
    size_t blocks_mined = 0;

    //kol dar yra transakciju mempoole, kasam blokus
    while (bc.mine_next_block(blocksz)) {
        ++blocks_mined;

        //grandines bukle po kiekvieno naujo bloko
        std::cout << "[chain] height=" << bc.chain().size()
                  << " mempool_left=" << bc.mempool().size()<< "\n";
    }

    //kai transakciju neliko ir daugiau nebera ka daryti
    std::cout << "[done] mining finished. blocks_mined=" << blocks_mined << "\n";

    
    //---------------------------------------------------------
    
    // INTERACTIVE MODE (query blocks/tx)
    // about a specific block or tx by id.
     std::cout << "\n[interactive] type 'help' for commands, 'exit' to quit\n";

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break; // EOF or ctrl+d
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "exit" || cmd == "quit") {
            std::cout << "[bye]\n";
            break;
        }

        if (cmd == "help") {
            std::cout
                << "Commands:\n"
                << "  help                - show this help\n"
                << "  exit                - quit\n"
                << "  getblock <height>   - show block by height (0 = genesis)\n"
                << "  gettx <txid>        - show transaction by id\n"
                << "  latest [n]          - show last n blocks (default 5)\n"
                << "  mempool [n]         - show first n tx from mempool (default 5)\n";
            continue;
        }

        if (cmd == "getblock") {
            size_t h;
            if (!(iss >> h)) {
                std::cout << "usage: getblock <height>\n";
                continue;
            }
            const Block* b = bc.get_block_by_height(h);
            if (!b) {
                std::cout << "Block not found (height=" << h << ")\n";
            } else {
                bc.print_block(*b, h); // pretty explorer-style dump
            }
            continue;
        }

        if (cmd == "gettx") {
            std::string txid;
            if (!(iss >> txid)) {
                std::cout << "usage: gettx <txid>\n";
                continue;
            }
            const Transaction* tx = bc.get_transaction_by_id(txid);
            if (!tx) {
                std::cout << "Transaction not found (txid=" << txid << ")\n";
            } else {
                bc.print_transaction(*tx);
            }
            continue;
        }

        if (cmd == "latest") {
            size_t n = 5;
            iss >> n;
            size_t total = bc.chain().size();
            size_t start = (total > n ? total - n : 0);

            for (size_t i = start; i < total; ++i) {
                const Block* b = bc.get_block_by_height(i);
                if (b) {
                    bc.print_block(*b, i);
                }
            }
            continue;
        }

        if (cmd == "mempool") {
            size_t n = 5;
            iss >> n;
            size_t shown = 0;
            for (const auto& tx : bc.mempool()) {
                if (shown++ >= n) break;
                bc.print_transaction(tx);
                std::cout << "----------------\n";
            }
            continue;
        }

        std::cout << "Unknown command: " << cmd
                  << " (type 'help')\n";
    }

    return 0;
}
