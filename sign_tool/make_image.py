import json
import hashlib
import argparse
from ecdsa import SigningKey

BIN_SIZE        = 0x20000
BOOT_OFFSET     = 0x1000
APP_OFFSET      = 0x10000
HASH_MAX_LEN    = 32
SIG_MAX_LEN     = 64
PUB_MAX_LEN     = 64

sign_key        = 0

def sign_image(image):
    global sign_key

    digest = hashlib.sha256(image).digest()
    return sign_key.sign_digest(digest, sigencode=lambda r, s, order: r.to_bytes(32, 'big') + s.to_bytes(32, 'big'))

parser = argparse.ArgumentParser()
parser.add_argument('-p', action='store_true', help='generate C source containing public key')
parser.add_argument('-f', action='store_true', help='customize image')
parser.add_argument('-t', action='store_true', help='test')
args = parser.parse_args()

if args.p:
    with open("private_key.pem", "rb") as f:
        sign_key = SigningKey.from_pem(f.read())

    line0 = "#include <stdint.h>"
    line1 = "\n\nconst uint8_t public_key_temp[] = {\n"
    line3 = "};\n"
    vk = sign_key.verifying_key
    public_key = vk.to_string()
    line2 = "    " + ", ".join(f'0x{b:02x}' for b in public_key) + "\n"

    with open("key_data.c", "w") as f:
        f.write(line0 + line1 + line2 + line3)

if args.f:
    final       = bytearray(BIN_SIZE)
    final[:]    = b'\xFF' * BIN_SIZE

    with open("private_key.pem", "rb") as f:
        sign_key = SigningKey.from_pem(f.read())

    with open("make_image.json", "r", encoding="utf-8") as f:
        json_data = json.load(f)
    
    for idx, fw in enumerate(json_data["firmware"]):

        file_bin        = fw["file_bin"]
        header_obj      = fw["header-data"]
        start_offset    = int(fw["start_offset"], 16)
        header          = int(header_obj["header"], 16)
        version_major   = int(header_obj["version_major"], 16)
        version_minor   = int(header_obj["version_minor"], 16)
        use_pubkey      = int(header_obj["use_pubkey"], 16)
        header_size     = int(header_obj["header_size"], 16)
        entry_point     = int(header_obj["entry_point"], 16)

        print(f"\nFirmware #{idx + 1}")
        print("  File BIN     :", file_bin)
        print("  Start Offset :", fw["start_offset"])
        print("  Header       :", header_obj["header"])
        print("  Major Version:", header_obj["version_major"])
        print("  Minor Version:", header_obj["version_minor"])
        print("  Use pubkey   :", header_obj["use_pubkey"])
        print("  Header Size  :", header_obj["header_size"])
        print("  Entry Point  :", header_obj["entry_point"])

        offset = start_offset

        with open(file_bin, "rb") as f:
            image = f.read()

        header_data = bytearray()

        # header
        header_data = header.to_bytes(4, 'little')

        # version_major
        header_data += version_major.to_bytes(4, 'little')

        # version_minor
        header_data += version_minor.to_bytes(4, 'little')

        # use_pubkey
        header_data += use_pubkey.to_bytes(4, 'little')

        # header_size
        header_data += header_size.to_bytes(4, 'little')

        # image_size
        image_size  = len(image)
        header_data += image_size.to_bytes(4, 'little')

        # image_offset
        if use_pubkey:
            image_offset = header_size + HASH_MAX_LEN + SIG_MAX_LEN + PUB_MAX_LEN
        else:
            image_offset = header_size + HASH_MAX_LEN + SIG_MAX_LEN
        
        header_data += image_offset.to_bytes(4, 'little')

        # entry_point
        header_data += entry_point.to_bytes(4, 'little')

        if use_pubkey:
            vk = sign_key.verifying_key
            public_key = vk.to_string()
            header_data += bytearray(public_key)

        final[offset:offset + len(header_data)] = header_data
        offset = offset + len(header_data)

        # Header hash
        header_hash = hashlib.sha256(header_data).digest()
        final[offset:offset + HASH_MAX_LEN] = header_hash
        offset = offset + HASH_MAX_LEN

        # Header Signature
        final[offset:offset + SIG_MAX_LEN] = sign_image(header_data)
        offset = offset + SIG_MAX_LEN

        # Image
        final[offset:offset + len(image)] = image
        offset = offset + len(image)

        # Image hash
        image_hash = hashlib.sha256(image).digest()
        final[offset:offset + HASH_MAX_LEN] = image_hash
        offset = offset + HASH_MAX_LEN

        # Signature hash
        final[offset:offset + SIG_MAX_LEN] = sign_image(image)
        offset = offset + SIG_MAX_LEN

    with open("firmware.bin", "wb") as f:
        f.write(final)
    
    with open("application.bin", "wb") as f:
        f.write(final[APP_OFFSET:offset])

