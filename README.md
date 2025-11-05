# Blockchain-2
# Supaprastinta blokų grandinė (Blockchain v0.2)

Tai yra **Bitcoin‑stiliaus blockchain** realizacija C++ kalba.  
Tikslas — suprasti kaip *iš tikro* veikia blokų grandinė, išardant ją iki atomų.

## Projektas apima:

| Funkcija | Aprašymas |
|---|---|
UTXO modelis | Transakcijų balanso tikrinimas
Mempool sistema | Transakcijų eilė
Proof‑of‑Work | Reikia rasti nonce → hash su nuliais
Coinbase TX | Nauji pinigai kasėjui
Fee sistema | Už TX kasėjui sumokama
Halving | Reward mažėja kas 50 blokų
Merkle Tree | Patikrinama transakcijų vientisumas
CLI | Galima tikrinti bloką / TX / balansą
Multi-candidate mining | (v0.2) 
Parallel kasimas su thread'ais ir stop flag | (v0.2) 

---

## Blokų grandinės komponentai

```
+---------------------+
|      Blockchain     |
|---------------------|
| chain (blocks[])    |
| mempool (tx queue)  |
| UTXO set            |
| difficulty          |
+---------+-----------+
          |
          |
+---------v-----------+
|      Mining         |
|---------------------|
| build candidates()  |
| PoW (nonce loop)    |
| multi-thread race   |
+---------+-----------+
          |
+---------v-----------+
|     Ledger/UTXO     |
|---------------------|
| validate tx inputs  |
| update balances     |
| prevent double spend|
+---------------------+
```

---

## 1. Projekto STRUKTŪRA

```
include/
    blockchain.h
    hash_adapter.h
src/
    blockchain_state.cpp      ← UTXO / balanso logika
    blockchain_mining.cpp     ← PoW ir kasimas
    blockchain_print.cpp      ← gražus blokų spausdinimas
    main.cpp                  ← CLI sąsaja
my_hash/
    hash.cpp                  ← mano hash algoritmas
    hash.h
```

---

## 2. Programos veikimo principas

Blockchain veikia tokiu ciklu:

1. Sukuriamas **genesis blokas**
2. Sugeneruojami vartotojai ir jų pradiniai UTXO
3. Sugeneruojamos transakcijos (mempool)
4. Imamos pirmos `block_size` transakcijos
5. Įdedama speciali `coinbase` transakcija kasėjui
6. Skaičiuojamas Merkle Root
7. Pradedamas PoW (nonce paieška)
8. Suradus tinkamą bloką — jis pridedamas į grandinę
9. Atnaujinamas UTXO rinkinys
10. Kartojama kol mempool tuščias

---

## 3. UTXO modelis (kaip Bitcoin)

UTXO = *Unspent Transaction Output* 

### Modelio schema:
```bash
        +-------------------+
        |   UTXO SET        |
        |-------------------|
    TxID:Index -> { owner, amount }
        +-------------------+

Naudojant TX:

Tx Input  -> sunaikina UTXO
Tx Output -> sukuria naują UTXO

```

### Principas

- Pinigai neegzistuoja kaip skaičius sąskaitoje
- Egzistuoja **neišnaudoti išėjimai (UTXO)**
- Transakcija:
  - sunaudoja senus UTXO
  - sukuria naujus

**Taisyklė:** `sum(input) ≥ sum(output)`

### VIZUALIAI:
```
UserA turi: (UTXO#1: 50)

Tx:
 Input:  UTXO#1:50
 Output: to B:30
         to A:20  (grąža)
```

### UTXO klaidos tikrinamos:

- Nėra input UTXO — klaida
- Bandymas išleisti jau panaudotą UTXO — klaida (double spend)
- `sum_out > sum_in` — klaida
- TX ID neatitinka VIN/VOUT hash — klaida (saugumas)

---

## 4. Bloko struktūra

### VIZUALIAI:
```bash
+---------------------------+
|        Block              |
|---------------------------|
| Header                    |
|  prev_block_hash          |
|  timestamp                |
|  difficulty               |
|  merkle_root              |
|  nonce                    |
|                           |
| Transactions[]            |
|  tx0 = coinbase           |
|  tx1, tx2, ...            |
+---------------------------+
```

Paaiškinimai:

| Laukas | Paaiškinimas |
|-------|--------------|
| prev_block_hash | bloko grandinės ryšys |
| timestamp | garantuoja laiko eiliškumą |
| txs_hash | Merkle Root — saugo transakcijų vientisumą |
| nonce | naudojama PoW iteracijoms |
| transactions[] | visos transakcijos (pirmoji — kasėjo atlygis) |

---

## 5. Proof‑of‑Work (PoW)

Tikslas rasti nonce, kad:

```
hash(block_header) prasidėtų difficulty pattern (pvz. "0000")
```

Procesas:

```
nonce = 0
while hash != valid:
    nonce++
```

---

## 6. Transakcijų generavimas

- Sukuriama `N` transakcijų
- Gavėjas = kitas vartotojas
- Suma 1–100
- TX ID = hash(VIN + VOUT)

---

## 7. Genesis blokas

- `prev_hash = 64 nuliai`
- `nonce = 0` (nekasamas)
- jokios transakcijos

Išvestis:

```
[chain] genesis block created (height=0)
```

---

## 8. Mempool

Laikinas transakcijų sąrašas iki patvirtinimo bloke.

```
mempool: tx1, tx2, tx3 ...
```

---

## 9. Coinbase + Halving

Pirmoji blokų transakcija:

```
miner_0 gauna reward
```

Reward mažėja kas 50 blokų:

```
50 → 25 → 12 → 6 → 3 → ... → min = 1
```

---

## 10. Merkle Root

Tikrinimo medžio šaknis iš visų `tx_id`.  
Užtikrina vientisumą — pakeitus bet kurią TX, keičiasi root.

### VIZUALIAI:
```BASH
TX0   TX1   TX2   TX3
 |     |     |     |
H0    H1    H2    H3
 |__ __|     |__ __|
   H01        H23
     |________|
        Root

Jei nelyginis → dubliuojam:

TX0 TX1 TX2
H0  H1  H2 H2  (duplicate last)
```
---

## 11. CLI režimas

Komandos:

Komanda | Paskirtis
-------|----------
`help` | Komandų sąrašas
`getblock N` | Parodo konkretų bloką
`gettx TXID` | Suranda transakciją
`latest N` | Rodo paskutinius blokus
`mempool N` | Rodo pirmas N mempool transakcijų
`exit` | Išeiti
---

## 12. v0.2 Lygiagretus kasimas (Multi‑Candidate Mining)

### v0.1

- 1 blokas generuojamas vienu metu
- 1 nonce ciklas

```
vienas kandidatas → ieškome nonce → radom
```

### v0.2

- Kuriami `num_candidates` nepriklausomi blokai
- Kiekvienam suteikiamas `max_ms` laikas
- Laimi pirmas radęs PoW

### Idėja:
```
5 kandidatų blokai
↓
visi bando rasti nonce
↓
pirmas rado → laimėjo → kiti nutraukiami
```

### VIZUALI EIGA:
```BASH
 ┌─────────────┐
 | Mempool TXs |
 └──────┬──────┘
  (take N)
       ↓
┌──────────────┐   spawn threads   ┌──────────────┐
| Candidate #1 |------------------>| Miner Thread |
└──────────────┘                   └──────────────┘
┌──────────────┐                   ┌──────────────┐
| Candidate #2 |------------------>| Miner Thread |
└──────────────┘                   └──────────────┘
┌──────────────┐                   ┌──────────────┐
| Candidate #3 |------------------>| Miner Thread |
└──────────────┘                   └──────────────┘
┌──────────────┐                   ┌──────────────┐
| Candidate #4 |------------------>| Miner Thread |
└──────────────┘                   └──────────────┘
┌──────────────┐                   ┌──────────────┐
| Candidate #5 |------------------>| Miner Thread |
└──────────────┘                   └──────────────┘

         winner!
            ↓
     commit → chain
```

### STOP mechanizmas:
```
atomic<bool> stop = false

if (thread finds PoW) {
    stop = true  // kiti sustoja
}
```

Argumentas:

```
--parallel
```

Tai imituoja realų tinklą, kur **kelios kasyklos konkuruoja**.

---

## 13. Paleidimas

### Įprastas

```bash
./blockchain 5000 50 0000
```

### Lygiagretus (v0.2)

```bash
./blockchain 5000 50 0000 --parallel
```

### Parametrai

| Argumentas | Reikšmė |
|--|--|
| 1 | transakcijų skaičius |
| 2 | bloko dydis (tx per bloką) |
| 3 | difficulty (nuliai stringe) |
| `--parallel` | aktyvuoja v0.2 |

---

## Hash funkcija

Naudojama custom hash:
- bubble sort + salt
- 256‑bit sėkla
- kiekvienas baito keitimas keičia visą hash

Hash adapteris konvertuoja į blockchain formatą.


## 14. Pavyzdinė išvestis

```
[mining] nonce=11071 hash=00065418e72b9ee3...
[mined] block found! nonce=11071...
[mined] block found! cand=2 nonce=18560 hash=0000ca928daa118a...

========================================
 Block #99
----------------------------------------
 block_hash:      0000ca928daa118aa572642898559188a6730096a356d1ce0319acc050be8c45
 prev_block_hash: 00008a40afdcd18cf6bb2d7cdab57663aede44a9234b20a36427135cfed91b51
 difficulty:      0000
 nonce:           18560
 timestamp:       1762253948
 txs_in_block:    50
 txs_hash:        0b24c6379ab2b1f614f26ff080a0aa17618f8d158c3b52ccd49e1013e27f1f67
----------------------------------------
 tx[0]: id=0056520730b04cf8...
   out[0]: to=miner_0... val=25
 tx[1]: id=9f996cb30b8ff821...
   in[0]: b49b6bd13cb844f8:1
   out[0]: to=4c6aa1a6dec65485... val=6
   out[1]: to=00e61328211b1102... val=4
 tx[2]: id=751e5d76b03bbb2a...
   in[0]: 0c27a2de151db38a:0
   out[0]: to=4c6aa1c5dec65888... val=10
   out[1]: to=b564d2d320e80030... val=68
 tx[3]: id=164f6fdddbce2f5c...
   in[0]: 0c27a2de151db38a:1
   out[0]: to=4c6aa1e4dec65c8b... val=95
   out[1]: to=682080ce8e6fb64a... val=539140
 tx[4]: id=f5f332009665f0cc...
   in[0]: 603d33506f38361f:0
   out[0]: to=4c6aa203dec6608e... val=25
   out[1]: to=b564d2f220e80433... val=35
 ... (45 more txs not shown)
========================================
[chain] height=100 mempool_left=149
```

---

## 15. Išvados

- Įgyvendintas Bitcoin‑stiliaus UTXO modelis
- Realizuotas PoW kasimas su difficulty
- Sudėtas Merkle Root patikrinimas
- Pridėta lygiagreti kasimo simuliacija (v0.2)
- Projektas tinka edukacijai ir PoW supratimui

---

