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
[mempool] Sukurta 10000 transakcijų
```

---

### 3. Transakcijų hash (`calc_txs_hash`)

Kai ruošiam naują bloką:
- Paimamos visos transakcijos, kurios bus tame bloke.
- Jų `tx_id` sujungiami į vieną ilgą eilutę.
- Tam tekstui apskaičiuojamas hash (vėl naudojant mano hash funkciją).

**Rezultatas:** `txs_hash` įdedamas į bloko header.

---

### 4. Bloko antraštės paruošimas (`serialize_header`)

Bloko antraštė paverčiama į vieną tekstinę eilutę:
```bash
prev_block_hash | timestamp | version | txs_hash | nonce | difficulty
```

**Pvz.:**
```bash
0000....0000|1730139065|v0.1|abc123ff9d...|84521|000
```

Ši eilutė hash’uojama, kad gautume `block_hash`.

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

**Progreso išvedimas (kas ~262k bandymų):**
```bash
[mining] nonce=381245 hash=000f1a4b32c9a01d...
```
**Kai randa:**
```bash
[mined] nonce=381245 hash=000f1a4b32c9a01d...
```

---

### 6. Blokas pridedamas prie grandinės

- Sukuriamas naujas `Block` objektas.  
- Priskiriami laukai: `header`, `txs`, `block_hash`.  
- Įterpiamas į `chain_`.  
- Transakcijos ištrinamos iš `mempool_` (jos jau bloke).  

---

### Block Explorer stiliaus išvedimas
Po kiekvieno sėkmingo kasimo išvedama bloko informacija:
```bash
========================================
 Block #100
----------------------------------------
 block_hash:      000c93d53e2b1d80...
 prev_block_hash: 000941ed8790e8ee...
 difficulty:      000
 nonce:           1127
 timestamp:       1730140123
 txs_in_block:    100
 txs_hash:        9a3bf12c7d...
----------------------------------------
 tx[0]: user_0 -> user_1 | amount=69 | nonce=9900 | id=8b018307c9bc6f45...
 tx[1]: user_1 -> user_2 | amount=72 | nonce=9901 | id=d2481fdd5d313e96...
 tx[2]: user_2 -> user_3 | amount=31 | nonce=9902 | id=4f67b772add5dca8...
 tx[3]: user_3 -> user_4 | amount=22 | nonce=9903 | id=4c81f2e6735a8605...
 tx[4]: user_4 -> user_5 | amount=36 | nonce=9904 | id=aab44ee5b413a846...
 ... (95 more txs not shown)
========================================

```

Tai leidžia aiškiai pamatyti:
1. Kiek transakcijų buvo bloke,
2. Kas buvo siuntėjai/gavėjai,
3. Koks nonce,
4. Koks PoW rezultatas (block_hash).

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
[config]  Txs=10000  BlockSize=100  Diff="000"
[start]   generuoju transakcijas...
[mempool] total 10000 transactions ready for mining
[start]   pradedu kasimą (Proof-of-Work)...

[mined] block found! nonce=1127 hash=000c93d53e2b1d80...
[block] applied: 100 txs added to block #101
[chain] height=102 mempool_left=0
========================================
 Block #101
----------------------------------------
 block_hash:      000c93d53e2b1d80...
 prev_block_hash: 000941ed8790e8ee...
 difficulty:      000
 nonce:           1127
 timestamp:       1730140123
 txs_in_block:    100
 txs_hash:        9a3bf12c7d...
----------------------------------------
 tx[0]: user_0 -> user_1 | amount=69 | nonce=9900 | id=8b018307c9bc6f45...
 ... (95 more txs not shown)
========================================
[done] Baigta. Iškasta blokų: 100
```

---

## AI pagalba
- Konsolės log'ų formavimui (aiškesni žingsniai).
- „block explorer“ peržiūros išvedimo idėjai.
