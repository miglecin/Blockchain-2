# tx_fee.py example
# Apskaičiuojame Bitcoin transakcijos mokestį pagal jos txid
# Naudojame python-bitcoinlib ir prisijungiame prie lokalaus Bitcoin Core nodo

from bitcoin.rpc import RawProxy

# Sukuriame prisijungimą prie vietinio Bitcoin Core mazgo
p = RawProxy()

# Transakcijos ID iš užduoties (2019-09-06 labai didelė transakcija)
txid = "4410c8d14ff9f87ceeed1d65cb58e7c7b2422b2d7529afc675208ce2ce09ed7d"

# 1. Gauname transakciją hex formatu
raw_tx = p.getrawtransaction(txid)

# 2. Dekoduojame transakciją į JSON objektą
decoded_tx = p.decoderawtransaction(raw_tx)

# 3. Suskaičiuojame visų išėjimų (vout) sumą
outputs_value = 0
for output in decoded_tx["vout"]:
    outputs_value += output["value"]

# 4. Suskaičiuojame visų įėjimų (vin) sumą:
#    reikia gauti ankstesnes transakcijas ir pasiimti atitinkamus vout
inputs_value = 0
for vin in decoded_tx["vin"]:
    prev_txid = vin["txid"]          # ankstesnės transakcijos ID
    prev_vout_index = vin["vout"]    # kurį išėjimą ji naudoja

    # gauname ankstesnę transakciją
    prev_raw = p.getrawtransaction(prev_txid)
    prev_decoded = p.decoderawtransaction(prev_raw)

    # pasiimame atitinkamą išėjimą ir jo value
    prev_output = prev_decoded["vout"][prev_vout_index]
