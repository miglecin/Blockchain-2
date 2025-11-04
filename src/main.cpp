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
#include <sstream>
#include <cstdlib>

int main(int argc, char** argv) {
    // numatytos reiksmes (jei vartotojas nieko neiveda)
    size_t n_txs   = 10000;   // kiek transakciju sugeneruoti i mempool
    size_t blocksz = 100;     // kiek transakciju viename bloke
    std::string diff = "000"; // PoW sunkumas (prefiksas)

    // CLI argumentai
    if (argc >= 2) n_txs   = std::strtoull(argv[1], nullptr, 10);
    if (argc >= 3) blocksz = std::strtoull(argv[2], nullptr, 10);
    if (argc >= 4) diff    = argv[3];

    std::cout << "  Txs=" << n_txs
              << "  BlockSize=" << blocksz
              << "  Diff=\"" << diff << "\"\n";

    // sukurti blockchain su pasirinktu sunkumu (v0.2 + UTXO)
    Blockchain bc(diff);

    // sugeneruoti ~1000 naudotoju ir pradinius UTXO
    std::cout << "[start] generating users...\n";
    bc.init_users(1000);

    // sugeneruoti UTXO pagrindu transakcijas i mempool
    std::cout << "[start] generating transactions (UTXO)...\n";
    bc.init_transactions(n_txs);

    // v0.2 kasybos parametrai
    size_t   num_candidates = 5;
    uint64_t max_ms         = 5000;

    std::cout << "[start] begin mining v0.2 (multi-candidate, time-limited, UTXO)...\n";
    size_t blocks_mined = 0;

    while (bc.mine_next_block_v2(blocksz, num_candidates, max_ms)) {
        ++blocks_mined;
        std::cout << "[chain] height=" << bc.chain().size()
                  << " mempool_left=" << bc.mempool().size() << "\n";
    }

    std::cout << "[done] mining finished. blocks_mined=" << blocks_mined << "\n";

    // INTERAKTYVUS REZIMAS (REPL)
    std::cout << "\n[interactive] type 'help' for commands, 'exit' to quit\n";
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd; iss >> cmd;

        if (cmd == "exit" || cmd == "quit") { std::cout << "[bye]\n"; break; }

        if (cmd == "help") {
            std::cout
                << "Commands:\n"
                << "  help                - show this help\n"
                << "  exit                - quit\n"
                << "  getblock <height>   - show block by height (0 = genesis)\n"
                << "  gettx <txid>        - show transaction by id (UTXO inputs/outputs)\n"
                << "  latest [n]          - show last n blocks (default 5)\n"
                << "  mempool [n]         - show first n tx from mempool (default 5)\n"
                << "  balance <pubkey>    - rodo balansa pagal pubkey (miner_0 irgi galioja)\n"
                << "  user <i>            - rodo i-to naudotojo info (name/pubkey/balance)\n";
            continue;
        }

        if (cmd == "getblock") {
            size_t h;
            if (!(iss >> h)) { std::cout << "usage: getblock <height>\n"; continue; }
            const Block* b = bc.get_block_by_height(h);
            if (!b) std::cout << "Block not found (height=" << h << ")\n";
            else bc.print_block(*b, h);
            continue;
        }

        if (cmd == "gettx") {
            std::string txid;
            if (!(iss >> txid)) { std::cout << "usage: gettx <txid>\n"; continue; }
            const Transaction* tx = bc.get_transaction_by_id(txid);
            if (!tx) std::cout << "Transaction not found (txid=" << txid << ")\n";
            else bc.print_transaction(*tx);
            continue;
        }

        if (cmd == "latest") {
            size_t n = 5; iss >> n;
            size_t total = bc.chain().size();
            size_t start = (total > n ? total - n : 0);
            for (size_t i = start; i < total; ++i) {
                const Block* b = bc.get_block_by_height(i);
                if (b) bc.print_block(*b, i);
            }
            continue;
        }

        if (cmd == "mempool") {
            size_t n = 5; iss >> n;
            size_t shown = 0;
            for (const auto& tx : bc.mempool()) {
                if (shown++ >= n) break;
                bc.print_transaction(tx);
                std::cout << "----------------\n";
            }
            continue;
        }

        if (cmd == "balance") {
            std::string pk;
            if (!(iss >> pk)) { std::cout << "usage: balance <pubkey|miner_0>\n"; continue; }
            uint64_t bal = 0;
            if (bc.get_balance(pk, bal)) {
                std::cout << "balance(" << pk.substr(0,16) << "...) = " << bal << "\n";
            } else {
                std::cout << "not found: " << pk << "\n";
            }
            continue;
        }

        if (cmd == "user") {
            size_t i;
            if (!(iss >> i)) { std::cout << "usage: user <index>\n"; continue; }
            bc.print_user(i);
            continue;
        }

        std::cout << "Unknown command: " << cmd << " (type 'help')\n";
    }

    return 0;
}
