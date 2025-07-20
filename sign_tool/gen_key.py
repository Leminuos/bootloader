from ecdsa import SigningKey, NIST256p

# Tạo private key
sk = SigningKey.generate(curve=NIST256p)

# Lưu private key vào PEM
with open("private_key.pem", "wb") as f:
    f.write(sk.to_pem())

# Lưu public key vào PEM
vk = sk.verifying_key
with open("public_key.pem", "wb") as f:
    f.write(vk.to_pem())
