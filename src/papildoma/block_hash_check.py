from bitcoin.rpc import RawProxy
import hashlib
import struct
import binascii

# Sukuriame prisijungimą prie vietinio Bitcoin Core nodo
p = RawProxy()

# Pasirenkame bloko aukštį (pvz., 100000)
blockheight = 100000

# Gauname bloko hash'ą pagal aukštį
blockhash = p.getblockhash(blockheight)

# Gauname pilną bloko informaciją
block = p.getblock(blockhash)

# Iš bloko paimame header'io laukus
version = block["version"]
prev_block_hash = block["previousblockhash"]
merkle_root = block["merkleroot"]
time = block["time"]
bits_hex = block["bits"]
nonce = block["nonce"]

# Konstruojame header'io baitus (little-endian)
header = b""
header += struct.pack("<L", version)
header += binascii.unhexlify(prev_block_hash)[::-1]
header += binascii.unhexlify(merkle_root)[::-1]
header += struct.pack("<L", time)
header += struct.pack("<L", int(bits_hex, 16))
header += struct.pack("<L", nonce)

# Double SHA-256
hash1 = hashlib.sha256(header).digest()