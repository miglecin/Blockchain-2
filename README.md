# Blockchain-2
# Supaprastinta blokų grandinė (Blockchain 2)

Šis projektas įgyvendina paprastą **blokų grandinės (blockchain)** modelį C++ kalba.
Kiekvienas blokas saugo transakcijų sąrašą ir turi savo **hash**, kuris jungiasi su ankstesniu bloku.
Naudojamas **Proof-of-Work** (PoW) principas – blokas laikomas galiojančiu, kai jo hash prasideda tam tikru kiekiu nulinių simbolių (`difficulty`).

---

## Pagrindinės savybės
- Mno sukurta `hash()` funkcija (failai `my_hash/hash.cpp`, `hash.h`).
- Transakcijų generavimas (naudojami paprasti `user_0`, `user_1`, ir t. t.).
- Blokų kasimas naudojant Proof-of-Work (`difficulty` pvz. `"000"`).
- Išsaugoma blokų grandinė su visais iškastais blokais.
- Aiškus struktūrinis padalijimas:
  - `src/` – pagrindinis programos kodas.
  - `include/` – antraštės failai.
  - `my_hash/` – mano hash algoritmas.

---
##  Paleidimas

```bash
# kompiliavimas
make

# paleidimas (numatytos reikšmės)
./blockchain

# arba su parametrais:
# ./blockchain [tx_kiekis] [bloko_dydis] [difficulty]
./blockchain 10000 100 000
```
---

##  Programos logika

### 1. Genesis blokas (pirmas)

Konstruktorius sukuria pirmaji **genesis bloką**:

- `prev_block_hash` = 64 nuliai (nes prieš jį nieko nėra)  
- `timestamp` = dabartinis laikas  
- `difficulty` = nurodytas sunkumas (pvz. `"000"`)  
- `txs_hash` = tuščias (genesis neturi transakcijų)  
- `nonce` = 0  
- `block_hash` = hash nuo header (nekasamas, nes pirmas)  

Blokas įterpiamas į grandinę (`chain_`).
**Konsolės išvestis:**
```bash
[chain] genesis block created (height=0)
```

---

### 2. Transakcijos (`init_transactions`)

Sugeneruojama nurodytas kiekis transakcijų (pvz. **10 000**):

- `sender` = `"user_" + skaičius`  
- `receiver` = `"user_" + (skaičius + 1)`  
- `amount` = atsitiktinis skaičius nuo 1 iki 100  
- `nonce` = transakcijos eilės numeris  
- `tx_id` sukuriamas naudojant **mano custom hash funkciją**  

Visi įrašai saugomi į `mempool_`, kuris veikia kaip laikinas sąrašas transakcijoms iki kol jos bus sudėtos į bloką.

**Konsolės išvestis:**
```bash
[mempool] generated 1000 / 10000 txs
[mempool] total 10000 transactions ready for mining
```

---

### 3. Coinbase / Block Reward transakcija

Kiekviename naujame iškastame bloke pirmoji transakcija yra speciali transakcija, vadinama block reward (arba „coinbase“ Bitcoin pasaulyje).

Ji atrodo taip:

```bash
tx[0]: Block_Reward -> miner_0 | amount=25 | nonce=1761732025 | id=a4ed48d59d39745e...
```

Svarbu:

- ši coinbase transakcija nėra paimta iš mempool, ji sukuriama automatiškai.
- ji įdedama kaip tx[0] į bloką prieš visas normalias transakcijas.
- ji yra naujų pinigų emisija (taip Bitcoin atsiranda nauji BTC).

Reward dydis nėra visada vienodas. Projekte yra realizuotas paprastas „halving“:

- bazinis atlygis yra 50,
- kas 50 blokų jis sumažinamas per pusę (50 → 25 → 12 → ...),
- niekada nenukrenta žemiau 1.

Tai imituoja tikrą Bitcoin ekonomiką, kur kas 4 metus vyksta „halving“.

### 4. Bloko formavimas

Kai ruošiam naują bloką:

1. Iš mempool_ paimame block_size transakcijų (pvz. 100).
2. Pridedame viršuje tx[0] – coinbase transakciją (reward kasėjui).
3. Sukuriame BlockHeader, kuriame yra:
  - prev_block_hash (nuoroda į praeitą bloką),
  - timestamp,
  - difficulty,
  - txs_hash (hash, gautas iš visų transakcijų ID, įskaitant coinbase),
  - nonce (pradžioje 0).

**Konsolėje:** 
```bash
[block] forming new block with 101 txs
```
---

### 5. Proof-of-Work (`mine_next_block`)

Tai pagrindinis kasimo ciklas:

1. Iš `mempool_` paimamos `block_size` transakcijos (pvz. 100).  
2. Sudaromas bloko header.  
3. Ciklas (`do...while`):
   - didina `nonce` nuo 1 iki begalybės,  
   - skaičiuoja `block_hash`,  
   - tikrina ar hash prasideda `difficulty` (pvz. `"000"`).  
4. Kai randamas tinkamas hash – blokas laikomas iškastas.

**Kasimo progresas:**
```bash
[mining] nonce=381245 hash=000f1a4b32c9a01d...
[mined]  block found! nonce=381245 hash=000f1a4b32c9a01d...
```

---

### 6. Blokas pridedamas prie grandinės

- Sukuriamas naujas `Block` objektas.  
- Įterpiamas į `chain_`.  
- Transakcijos ištrinamos iš `mempool_` (jos jau bloke).  

**Konsolės išvestis:**
```bash
[block] applied: 100 txs added to block #42
[chain] height=43 mempool_left=5700
```
---

### Block Explorer stiliaus išvedimas
Po kiekvieno sėkmingo kasimo išvedama bloko informacija:
```bash
========================================
 Block #88
----------------------------------------
 block_hash:      00018d3dc0cbd8bbac5bd2fd71c1cb7d4d8fa2884d4ee8aca7d4485025f82a86
 prev_block_hash: 000e0f5b5563a19563b75767862054a6d7645dc6e272f6dc498816f8777d717d
 difficulty:      000
 nonce:           6167
 timestamp:       1761732025
 txs_in_block:    101
 txs_hash:        8b393c123103e8fb25713d27d2905c0e502ae9e4f0d822bfd188b3abfde15210
----------------------------------------
 tx[0]: Block_Reward -> miner_0 | amount=25 | nonce=1761732025 | id=a4ed48d59d39745e...
 tx[1]: user_0 -> user_1 | amount=37 | nonce=8700 | id=f80262dc5b37d4b7...
 tx[2]: user_1 -> user_2 | amount=4  | nonce=8701 | id=c355b1db85f8fa34...
 tx[3]: user_2 -> user_3 | amount=100 | nonce=8702 | id=06c1339109ae726b...
 tx[4]: user_3 -> user_4 | amount=25 | nonce=8703 | id=56089f16777fee2b...
 ... (96 more txs not shown)
========================================

```

Tai leidžia aiškiai pamatyti:
1. Kiek transakcijų buvo bloke,
2. Kas buvo siuntėjai/gavėjai,
3. Koks nonce,
4. Koks PoW rezultatas (block_hash),
5. Kokia block reward transakcija (kasėjas gauna pinigus).

----

### Interaktyvus režimas (užklausos)

- *help*	- Parodo visų komandų sąrašą
- *getblock <n>*	- Parodo konkretų bloką (0 = genesis)
- *gettx <txid>*	- Parodo transakciją pagal ID
- *latest [n]*	- Parodo paskutinius n blokus
- *mempool [n]*	- Parodo pirmas n transakcijų iš mempool
- *exit* - Išeina iš programos

```bash
> getblock 5
> gettx 8b018307c9bc6f45...
> latest 3
> mempool 10
> exit
```

---

##  Hash funkcija

Naudoja **bubble sort** algoritmą, kuris kiekvieno sukeitimo metu atnaujina 256-bit sėklą (`seed`).  
Kiekvienas baitų sukeitimas keičia hash, todėl net maža įvesties permaina duoda visiškai kitokį rezultatą.

**Pagrindiniai komponentai:**
- `bubble_sort_and_hash` – pagrindinė maišos funkcija  
- `make_salt` – sukuria 16 baitų druską iš pranešimo  
- `hash_to_hex` – konvertuoja hash į šešioliktainį tekstą  

---

##  Klasės ir struktūros

| Pavadinimas | Paskirtis |
|--------------|------------|
| **Transaction** | Saugo siuntėją, gavėją, sumą, nonce ir `tx_id` |
| **BlockHeader** | Saugo informaciją apie bloką (hash, timestamp, difficulty, nonce ir kt.) |
| **Block** | Vienas blokas su antrašte ir transakcijomis |
| **Blockchain** | Valdo grandinę, kasimą, `mempool` ir hash’us |
| **HashAdapter** | Jungia mano hash funkciją su `Blockchain` klasėmis |

---

### Konsolės išvestis (pvz.)
```bash
./blockchain 10000 100 000
```
```bash
Txs=10000  BlockSize=100  Diff="000"
[start] generuoju transakcijas...
[mempool] generated 1000 / 10000 txs
[mempool] generated 2000 / 10000 txs
[mempool] generated 3000 / 10000 txs
[mempool] generated 4000 / 10000 txs
[mempool] generated 5000 / 10000 txs
[mempool] generated 6000 / 10000 txs
[mempool] generated 7000 / 10000 txs
[mempool] generated 8000 / 10000 txs
[mempool] generated 9000 / 10000 txs
[mempool] total 10000 transactions ready for mining
[start] pradedu kasimą (Proof-of-Work)...
```

```bash
[reward] 50 coins issued by Block_Reward to miner_0
[block] forming new block with 101 txs
[mining] nonce=262144 hash=000a32f89c1bd2ff...
[mined] block found! nonce=268101 hash=000a32f89c1bd2ff...
[block] applied: 101 txs added to block #1
[chain] height=2 mempool_left=9899
```

```bash
========================================
 Block #1
----------------------------------------
 block_hash:      000a32f89c1bd2ffb71a410b64e8e5b79e6de2100b9a76db02bce33ce45a9e40
 prev_block_hash: 0000000000000000000000000000000000000000000000000000000000000000
 difficulty:      000
 nonce:           268101
 timestamp:       1761731622
 txs_in_block:    101
 txs_hash:        8b393c123103e8fb25713d27d2905c0e502ae9e4f0d822bfd188b3abfde15210
----------------------------------------
 tx[0]: Block_Reward -> miner_0 | amount=50 | nonce=1761731622 | id=f37ac314e6c6a1a1...
 tx[1]: user_0 -> user_1 | amount=37 | nonce=0 | id=fa5bd67b96a40c57...
 tx[2]: user_1 -> user_2 | amount=91 | nonce=1 | id=2120db97a8c63e88...
 tx[3]: user_2 -> user_3 | amount=12 | nonce=2 | id=07fdac8f9f32a924...
 tx[4]: user_3 -> user_4 | amount=84 | nonce=3 | id=16be51ed3f07c9d2...
 ... (96 more txs not shown)
========================================
```
.....

```bash
[done] Baigta. Iškasta blokų: 100
```

---

## Kodėl reikalingas block reward?

- Proof-of-Work reikalauja darbo (nonce paieškos).
- Kasėjas turi gauti užmokestį už šitą darbą.
- Bloke automatiškai sugeneruojama coinbase transakcija:
- Block_Reward -> miner_X | amount=...
- Taip į tinklą „įleidžiami“ nauji pinigai.
- Reward suma mažėja kas 50 blokų (halving mechanizmas), kaip Bitcoin

---

## AI pagalba
- Konsolės log'ų formavimui (aiškesni žingsniai).
- „block explorer“ peržiūros išvedimo idėjai.
- coinbase/block reward idėjai ir halving logikai.
