# Blockchain-2
# Supaprastinta blokų grandinė (Blockchain 2)

Šis projektas įgyvendina paprastą **blokų grandinės (blockchain)** modelį C++ kalba.
Kiekvienas blokas saugo transakcijų sąrašą ir turi savo **maišą (hash)**, kuris jungiasi su ankstesniu bloku.
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

