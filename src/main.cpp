// CLI usage:
//   ./blockchain [n_txs] [block_size] [difficulty]
//
// examples:
//   ./blockchain
//   ./blockchain 5000
//   ./blockchain 5000 50
//   ./blockchain 5000 50 000 --v2
//   ./blockchain 10000 100 000 --v2 --parallel --candidates 8 --time-ms 3000

// flags:
//   --v1               - kasa senu vieno kandidato rezimu (be laiko limito)
//   --v2               - v0.2 paprastas multi-candidate (su laiko limitu)
//   --parallel         - v0.2 lygiagretus multi-candidate (threads)
//   --candidates <N>   - kandidatu skaicius (default 5)
//   --time-ms <MS>     - laiko limitas kasybai vienai bangai (default 5000)
#include "blockchain.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

int main(int argc, char** argv) {
    // numatytos reiksmes (jei vartotojas nieko neiveda)
    size_t n_txs   = 10000;   // kiek transakciju sugeneruoti i mempool
    size_t blocksz = 100;     // kiek transakciju viename bloke
    std::string diff = "000"; // PoW sunkumas (prefiksas)

    // kasybos rezimas (vienas is: v1, v2, parallel)
    enum Mode { MODE_V1, MODE_V2, MODE_V2_PARALLEL };
    Mode mode = MODE_V2; // default v0.2 paprastas

    // v0.2 parametrai
    size_t num_candidates = 5;
    uint64_t max_ms = 5000;

    // CLI argumentai
    if (argc >= 2) n_txs   = std::strtoull(argv[1], nullptr, 10);
    if (argc >= 3) blocksz = std::strtoull(argv[2], nullptr, 10);
    if (argc >= 4) diff    = argv[3];

    //papildomi flagai
    for (int i = 4; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--v1") {
            mode = MODE_V1;
        } else if (a == "--v2") {
            mode = MODE_V2;
        } else if (a == "--parallel") {
            mode = MODE_V2_PARALLEL;
        } else if (a == "--candidates" && i + 1 < argc) {
            num_candidates = std::strtoull(argv[++i], nullptr, 10);
        } else if (a == "--time-ms" && i + 1 < argc) {
            max_ms = std::strtoull(argv[++i], nullptr, 10);
        } else if (a == "--help" || a == "-h") {
            std::cout
                << "Usage: ./blockchain [n_txs] [block_size] [difficulty] [flags]\n"
                << "Flags:\n"
                << "  --v1                kasa vieno kandidato rezimu (be laiko limito)\n"
                << "  --v2                paprastas multi-candidate (default)\n"
                << "  --parallel          lygiagretus multi-candidate (threads)\n"
                << "  --candidates <N>    kandidatu skaicius (default 5)\n"
                << "  --time-ms <MS>      laiko limitas vienai bangai (default 5000)\n";
            return 0;
        }
    }

    // isvedam pasirinkimus
    std::cout << "  Txs=" << n_txs
              << "  BlockSize=" << blocksz
              << "  Diff=\"" << diff << "\""
              << "  Mode=" << (mode == MODE_V1 ? "v1"
                                   : mode == MODE_V2 ? "v2"
                                                     : "v2_parallel")
              << "  Candidates=" << num_candidates
              << "  TimeMs=" << max_ms
              << "\n";

    //sukurti blockchain 
    Blockchain bc(diff);

    //v0.2: naudotojai + UTXO
    std::cout << "[start] generating users...\n";
    bc.init_users(1000);

    std::cout << "[start] generating transactions...\n";
    bc.init_transactions(n_txs);

    //kasyba pagal pasirinkta rezima
    size_t blocks_mined = 0;
    if (mode == MODE_V1) {
        std::cout << "[start] begin mining v0.1 (single candidate)...\n";
        while (bc.mine_next_block(blocksz)) {
            ++blocks_mined;
            std::cout << "[chain] height=" << bc.chain().size()
                      << " mempool_left=" << bc.mempool().size() << "\n";
        }
    } else if (mode == MODE_V2) {
        std::cout << "[start] begin mining v0.2 (multi-candidate, time-limited)...\n";
        while (bc.mine_next_block_v2(blocksz, num_candidates, max_ms)) {
            ++blocks_mined;
            std::cout << "[chain] height=" << bc.chain().size()
                      << " mempool_left=" << bc.mempool().size() << "\n";
        }
    } else { // MODE_V2_PARALLEL
        std::cout << "[start] begin mining v0.2 (PARALLEL multi-candidate, time-limited)...\n";
        while (bc.mine_next_block_v2_parallel(blocksz, num_candidates, max_ms)) {
            ++blocks_mined;
            std::cout << "[chain] height=" << bc.chain().size()
                      << " mempool_left=" << bc.mempool().size() << "\n";
        }
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
                << "  getblocktx <height> <index> - show transaction by id (UTXO inputs/outputs)\n"
                << "  latest [n]          - show last n blocks (default 5)\n"
                << "  mempool [n]         - show first n tx from mempool (default 5)\n"
                << "  balance <name OR pubkey> - rodo balansa pagal pubkey (miner_0 irgi galioja\n"
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

        if (cmd == "getblocktx") {
            size_t h, idx;
            if (!(iss >> h >> idx)) {
                std::cout << "usage: getblocktx <height> <index>\n";
            } else {
                const Block* b = bc.get_block_by_height(h);
                if (!b) { std::cout << "no such block\n"; }
                else if (idx >= b->txs.size()) { std::cout << "tx index out of range\n"; }
                else {
                    const Transaction& tx = b->txs[idx];
                    bc.print_transaction(tx); // čia atspausdina pilną tx_id, VIN/VOUT
                }
            }
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
    std::string key;
    iss >> key;

    if (key.empty()) {
        std::cout << "Usage: balance <pubkey or username>\n";
        continue;
    }

    // jei įvestas vartotojo vardas – ieškome jo pubkey
    for (const auto& u : bc.users()) {
        if (u.name == key) {
            key = u.pubkey;
            break;
        }
    }

    uint64_t bal = 0;
    if (bc.get_balance(key, bal)) {
        std::cout << "Balance: " << bal << "\n";
    } else {
        std::cout << "User / key not found\n";
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
