#include "blockchain.h"
#include <iostream>
#include <algorithm>

//vidine pagalbine funkcija
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
            std::cout << "   in[" << j << "]: " << in.prev.txid.substr(0,16)
                      << ":" << in.prev.index << "\n";
        }
        for (size_t k = 0; k < tx.vout.size(); ++k) {
            const TxOut& out = tx.vout[k];
            std::cout << "   out[" << k << "]: to=" << out.owner.substr(0,16)
                      << "... val=" << out.value << "\n";
        }
    }
    if (b.txs.size() > preview_count) {
        std::cout << " ... (" << (b.txs.size() - preview_count) << " more txs not shown)\n";
    }
    std::cout << "========================================\n\n";
}

void Blockchain::print_block(const Block& b, size_t height) const {
    print_block_pretty(b, height);
}
