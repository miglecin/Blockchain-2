// ============================================================
// merkle.cpp
// 3-asis papildomas praktinis darbas:
// Libbitcoin naudojimas Merkle šaknies (Merkle Root) skaičiavimui
// ============================================================

#include <bitcoin/system.hpp>   // Pagrindinė Libbitcoin antraštė
#include <iostream>

// Naudosime trumpinį „bc“, kad vietoje libbitcoin::system rašytume tiesiog bc
namespace bc = libbitcoin;

// ------------------------------------------------------------
// Funkcija: create_merkle
// Aprašymas:
//  Apskaičiuoja Merkle Root reikšmę iš transakcijų hash sąrašo.
// ------------------------------------------------------------
bc::hash_digest create_merkle(bc::hash_list& merkle)
{
    // Jei sąrašas tuščias, grąžiname nulinį hash
    if (merkle.empty())
        return bc::null_hash;

    // Jei sąraše tik vienas hash, tai jis ir yra Merkle Root
    if (merkle.size() == 1)
        return merkle[0];

    // Kol hash sąraše daugiau nei vienas elementas, tęsiame jungimą
    while (merkle.size() > 1)
    {
        // Jei hash'ų skaičius nelyginis, paskutinį elementą dubliuojame
        if (merkle.size() % 2 != 0)
            merkle.push_back(merkle.back());

        // Sukuriame naują (tuščią) Merkle sąrašą
        bc::hash_list new_merkle;

        // Einame per sąrašą poromis (po 2 hash’us)
        for (auto it = merkle.begin(); it != merkle.end(); it += 2)
        {
            // Sukuriame buferį, kuriame sujungiami 2 hash’ai (po 32 baitus)
            bc::data_chunk concat_data(bc::hash_size * 2);

            // Paruošiame serializatorių rašyti duomenis į buferį
            auto concat = bc::serializer<decltype(concat_data.begin())>(concat_data.begin());

            // Įrašome abu hash’us į tą patį duomenų buferį
            concat.write_hash(*it);
            concat.write_hash(*(it + 1));

            // Atliekame dvigubą SHA256 hash’ą (Bitcoin stiliaus)
            bc::hash_digest new_root = bc::bitcoin_hash(concat_data);

            // Gautą hash’ą pridedame į naują sąrašą
            new_merkle.push_back(new_root);
        }

        // Dabartinį Merkle sąrašą pakeičiame nauju
        merkle = new_merkle;

        // --- (Nebūtina, bet naudinga mokymuisi) ---
        // Išvedame tarpinius rezultatus, kad matytume, kaip kinta sąrašas
        std::cout << "Tarpinis Merkle hash sąrašas:" << std::endl;
        for (const auto& hash : merkle)
            std::cout << "  " << bc::encode_base16(hash) << std::endl;
        std::cout << std::endl;
        // ------------------------------------------
    }

    // Grąžiname vienintelį likusį hash — tai ir yra Merkle Root
    return merkle[0];
}

// ------------------------------------------------------------
// Pagrindinė programa
// ------------------------------------------------------------
int main()
{
    // Bitcoin bloko #100000 transakcijų hash’ai
    // Šie duomenys naudojami norint gauti žinomą Merkle Root
    bc::hash_list tx_hashes{{
        bc::hash_literal("8c14f0db3df150123e6f3dbbf30f8b955a8249b62ac1d1ff16284aefa3d06d87"),
        bc::hash_literal("fff2525b8931402dd09222c50775608f75787bd2b87e56995a7bdd30f79702c4"),
        bc::hash_literal("6359f0868171b1d194cbee1af2f16ea598ae8fad666d9b012c8ed2b79a236ec4"),
        bc::hash_literal("e9a66845e05d5abc0ad04ec80f774a7e585c6e8db975962d069a522137b80c1d"),
    }};

    // Apskaičiuojame Merkle Root kviesdami savo funkciją
    const bc::hash_digest merkle_root = create_merkle(tx_hashes);

    // Išvedame galutinį Merkle Root hash į ekraną (hex formatu)
    std::cout << "Merkle Root Hash: " << bc::encode_base16(merkle_root) << std::endl;

    // (Papildomai) galime parodyti ir alternatyvų formatą:
    // std::cout << "Merkle Root Hash (encoded): " << bc::encode_hash(merkle_root) << std::endl;

    return 0;
}
