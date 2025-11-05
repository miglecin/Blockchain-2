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

## 2. Programos veikimo PRINCIPAS

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

| Laukas            | Paaiškinimas                                            | Kodėl svarbu                                                 |
| ----------------- | ------------------------------------------------------- | ------------------------------------------------------------ |
| `prev_block_hash` | Nuoroda į ankstesnį bloką                               | Užtikrina grandinės vientisumą — negalima pakeisti istorijos |
| `timestamp`       | UNIX laikas, kada blokas sukurtas                       | Laiko seka + apsauga nuo pakartotinio naudojimo              |
| `difficulty`      | Kiek nulinių bitų turi prasidėti bloko hash             | Nustato kasimo sudėtingumą ir tinklo saugumą                 |
| `merkle_root`     | Hash iš visų transakcijų bloko viduje                   | Užtikrina, kad nė viena TX negali būti pakeista              |
| `nonce`           | Skaičius, kurį kasa miner'iai, kad gautų tinkamą hash | Proof-of-Work — garantuoja, kad *difficulty* atitinka      |
| `Transactions[]`  | Visos bloko transakcijos                                | Perduoda monetų valdymą (UTXO sunaudojami ir sukuriami nauji)                                 |
| `tx0 = coinbase`  | Speciali transakcija — bloko atlygį gauna kasėjas       | Sukuria naujus coin'us + surenka fees                        |
| `tx1, tx2...`     | Registruoja pinigų judėjimą tarp vartotojų (sumažina siuntėjo balansą ir padidina gavėjo balansą)                        | Užtikrina teisingą balansų apskaitą ir transakcijų istoriją                      |

---

## 5. Proof‑of‑Work (PoW)

**Proof-of-Work (PoW)** yra algoritmas, kuris užtikrina, kad naujas blokas būtų **sukurtas tik atlikus realų skaičiavimo darbą**.
Tai apsaugo tinklą nuo piktavalių ir dvigubo išleidimo (double-spend).

#### Tikslas:
Rasti tokį `nonce`, kad:
```
hash(block_header) prasidėtų difficulty (pvz. "0000")
```
Tai vadinama **kasimu**, nes kompiuteris turi "iškasti" tinkamą nonce vertę.

#### Pseudokodas:
```
nonce = 0
do {
    hash = HASH(header + nonce)
    nonce++
} while (hash does not start with "000...")
```
#### Ką garantuoja PoW?
| Užtikrina        | Ką reiškia                                          |
| ---------------- | --------------------------------------------------- |
| Decentralizaciją | Bet kas gali kasti, nėra centrinio valdovo          |
| Saugumą          | Kad pakeisti bloką, reikia perkasti visus sekančius |
| İntegnumą        | Nė vienas negali į tinklą įterpti suklastotų blokų  |
| Anti-spam        | Blokų kūrimas kainuoja energiją / laiką             |

### VIZUALIAI:
```BASH
Header + Nonce ---> HASH ---> Prasideda "000"? → Taip → Blokas tvirtas
                          \→ Ne → nonce++
```
---

## 6. Transakcijos

Transakcijos šiame projekte modeliuoja **pinigų judėjimą tarp vartotojų**, panašiai kaip Bitcoin tinklo transakcijos.

### Kas yra transakcija?
Transakcija nurodo:
  - kas siunčia lėšas (`sender`)
  - kam siunčiamos lėšos (`receiver`)
  - kiek monetų siunčiama (`amount`)
  - iš kur pasiimamos lėšos (UTXO input'ai)
  - į kokius naujus UTXO jos išskaidomos (output'ai)

Trumpai — tai **nuosavybės perleidimo instrukcija**.

#### TX generavimas:
Programa automatiškai sukuria `N` transakcijų:
  - Pasirenkamas atsitiktinis siuntėjas (su likučiu)
  - Pasirenkamas atsitiktinis gavėjas
  - Parenkama atsitiktinė suma (pvz. 1–100 monetų)
  - Surenkami siuntėjo UTXO padengti sumai
  - Sukuriami išėjimai: gavėjui + grąža siuntėjui
  - Apskaičiuojamas transakcijos ID (hash)

#### Schema:
```bash
Inputs (UTXO) ----> Transaction ----> New Outputs (UTXO)
```

---

## Transakcijos ID (TXID)

Kad transakcijos būtų unikalios ir neklastojamos, joms sugeneruojamas hash:
```
TXID = HASH( inputs + outputs + amount + nonce + timestamp )
```
Naudojama mano **custom hash funkcija**.

#### Kodėl reikia TXID?
TX ID yra:
  - transakcijos identifikatorius (kaip serijos numeris)
  - nuoroda į UTXO input'us
  - įrodymas, kad transakcija nebuvo pakeista

TXID veikia kaip skaitmeninis **parašas ir štampas viename**.

---

## Mempool

Sugeneruotos transakcijos **patenka į mempool** — laukiančių transakcijų eilę:
```
TxPool = { tx1, tx2, tx3, ... }
```
Mineris pasiima pirmas `block_size` transakcijų ir bando sudėti jas į bloką.

#### Pseudokodas:
```
for i in range(N):
    sender   = random user with UTXO
    receiver = random different user
    amount   = random(1..100)

    inputs = pick UTXO that cover amount
    outputs = {
        receiver: amount,
        sender: change_if_any
    }

    tx.tx_id = hash(inputs + outputs + timestamp)
    mempool.push(tx)
```

---

## Transakcijų mokesčiai (Fees)

Kiekviena transakcija turi fiksuotą mokestį — `1 coin`
  - Mokesčiai keliauja į miner coinbase reward'ą
  - Tai imituoja tikrą Bitcoin fee sistemą

--- 

## 8. Mempool

- `prev_hash = 64 nuliai`
- `nonce = 0` (nekasamas)
- jokios transakcijos

Išvestis:

```
[chain] genesis block created (height=0)
```
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

