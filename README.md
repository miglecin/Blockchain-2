# Blockchain-Papildoma
### Libbitcoin ir python-bitcoinlib naudojimas

---
## Užduoties tikslas

Šio papildomo praktinio darbo tikslas – susipažinti su Bitcoin blokų grandinės (Blockchain) technologijos žemesnio lygmens įrankiais ir bibliotekomis:

- **Libbitcoin** – C++ biblioteka darbui su Bitcoin duomenų struktūromis (blokais, transakcijomis, skriptais).
- **python-bitcoinlib** – Python kalbos biblioteka, suteikianti prieigą prie tų pačių konceptų, bet aukštesnio lygmens programavimo aplinkoje.

Darbo metu buvo įgyvendintas praktinis pavyzdys – Merkle Root hash’o skaičiavimas naudojant Libbitcoin, o taip pat išbandyta Python biblioteka.

---

## 1. Aplinkos paruošimas

Pradžioje bandyta kompiliuoti `libbitcoin-system` tiesiogiai macOS aplinkoje, tačiau kilo keli nesklandumai:

- `brew install libbitcoin-system` nepavyko, nes paketas buvo pašalintas dėl pasenusio `boost@1.76`.
- Naudojant `install.sh` skriptą, `include/bitcoin` aplanke neatsirado reikalingų antraščių failų.

Sprendimas – naudoti Docker (Ubuntu 22.04) aplinką, kur galima tiksliai valdyti priklausomybes.

---

## 2. Docker konteinerio kūrimas

Konteinerio paleidimas iš Mac terminalo:
```bash
docker run -it --name libbtc ubuntu:22.04 bash
```

Viduje įdiegti reikalingi įrankiai:
```bash
apt update
apt install -y git build-essential autoconf automake libtool pkg-config wget curl
```

---

## 3. Priklausomybių diegimas

#### Boost biblioteka
```bash
apt install -y libboost-all-dev
```

#### secp256k1 biblioteka
Naujausioje versijoje buvo pašalintos funkcijos `(secp256k1_ec_privkey_tweak_add/mul)`, todėl naudotas senesnis commit:

```bash
cd /root
git clone https://github.com/bitcoin-core/secp256k1.git
cd secp256k1
git checkout ac83be33
./autogen.sh
./configure --enable-module-recovery --prefix=/usr
make -j$(nproc)
make install
```

---

## 4. Libbitcoin-System diegimas

```bash
cd /root
git clone https://github.com/libbitcoin/libbitcoin-system.git
cd libbitcoin-system
./autogen.sh
./configure --prefix=/usr
make -j$(nproc)
make install
```

Po įdiegimo atsiranda:
```bash
/usr/include/bitcoin/system.hpp
/usr/include/bitcoin/system/
```

---

## 5. Programos `merkle.cpp` kūrimas

Programa apskaičiuoja Merkle Root iš keturių Bitcoin transakcijų hash’ų (pvz., iš bloko #100000).

Failas sukuriamas konteineryje:
```bash
cd /root
cat > merkle.cpp << 'EOF'
// (čia įklijuotas pilnas kodas)
EOF
```
Kompiliavimas:
```bash
g++ -std=c++17 merkle.cpp -o merkle -lbitcoin-system
```

---

## 6. Paleidimas ir rezultatas

Programos vykdymas:
```bash
./merkle
```

Rezultatas:
```bash
Tarpinis Merkle hash sąrašas:
  15b88c5107195bf09eb9da89b83d95b3d070079a3c5c5d3d17d0dcd873fbdacc
  49aef42d78e3e9999c9e6ec9e1dddd6cb880bf3b076a03be1318ca789089308e

Tarpinis Merkle hash sąrašas:
  6657a9252aacd5c0b2940996ecff952228c3067cc38d4885efb5a4ac4247e9f3

Merkle Root Hash: 6657a9252aacd5c0b2940996ecff952228c3067cc38d4885efb5a4ac4247e9f3
```

---

## 7. Klaidos ir sprendimai

| Etapas                                 | Problema                           | Sprendimas                          |
| -------------------------------------- | ---------------------------------- | ----------------------------------- |
| macOS `brew install libbitcoin-system` | Tap nebegalimas                    | Pereita prie Docker                 |
| Kompiliuojant `libbitcoin-system`      | `secp256k1` neatitiko API          | Naudotas senesnis commit `ac83be33` |
| `namespace libbitcoin::system` klaida  | Naujesnė `libbitcoin` versija      | Pakeista į `namespace libbitcoin`   |
| `docker cp` klaida                     | Komanda vykdyta konteinerio viduje | Paleista iš Mac terminalo           |

---

## Testo duomenų keitimas (3 užduotis)

Pagal užduoties reikalavimus galima pakeisti pavyzdinius transakcijų hash’us į **realius Bitcoin transakcijų identifikatorius (txid)** iš pasirinkto bloko.
Tam galima naudoti bet kurį **blockchain explorer**, pvz. https://www.blockchain.com/explorer

1. Atsidaryti norimą bloką (pvz., #800000).
2. Nukopijuoti kelis transakcijų hash’us (txid).
3. Įkelti juos į merkle.cpp vietoje esamų pavyzdinių hash’ų.

Pavyzdžiui:
```
std::vector<bc::hash_digest> tx_hashes = {
    bc::hash_literal("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
    bc::hash_literal("b2f5ff47436671b6e533d8dc3614845d5b6a5c3b9a5b56b8a8a2e1b4d5f3bba3"),
    bc::hash_literal("c7be1ed902fb3d6c36c1e60ffb3a3aa7bcb76e54d2cb3f8c2a8d7e3b5b8aab09"),
    bc::hash_literal("88e6c3bdfbb3af3c8a81e73b14a9a0f0bfc54d5e26d4221a53ff32b6ab3f9e8f")
};
```
