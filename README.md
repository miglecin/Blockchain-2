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

##  Viso proceso SANTRAUKA

1. Sukuriamas **genesis blokas**  
2. Sugeneruojamos **transakcijos** (`init_transactions`)  
3. **Kasamas blokas** (`mine_next_block`)  
4. Kasimo metu randamas `nonce`, kuris duoda hash su prefiksu `"000"`  
5. Blokas pridedamas prie grandinės  
6. Procesas kartojamas, kol `mempool_` tuščias

**Galutinė išvestis:**
```bash
Users=1000 Txs=10000 BlockSize=100 Diff="000"
[mempool] Sukurta 10000 transakcijų
[kasimas] nonce=382144 hash=000f3a9bd7d83e41...
[rasta] nonce=382144 hash=000f3a9bd7d83e41...
[chain] aukštis=2 liko_transakcijų=9900
...
Baigta. Iškasta blokų: 100

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
