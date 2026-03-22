#!/usr/bin/env python3
"""
Parse and verify **stm32_secure_boot** signed application images (sign_image.py format).

Format (see sibling repo ``stm32_secure_boot/scripts/sign_image.py``):
  - 96-byte header: magic, version, image_size, entry, reserved[16], ECDSA signature[64]
  - raw application bytes (SHA-256 over this payload is signed)

Uses **ECDSA secp256k1** with PEM public key — **not** X.509 certificates in the current tooling.

Host-only (Python + ``ecdsa``). Optional dependency for tests / manual checks.
"""
from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

IMAGE_HEADER_MAGIC = 0x424F4F54  # 'BOOT'
# Must match stm32_secure_boot/scripts/sign_image.py
APP_ENTRY_POINT_DEFAULT = 0x08010080
HEADER_SIZE = 96


@dataclass(frozen=True)
class SignedImageParts:
    magic: int
    version: int
    image_size: int
    entry: int
    signature: bytes  # 64 bytes (raw r||s)
    app: bytes


def sign_app_bytes(
    app: bytes,
    privkey_pem: bytes,
    *,
    app_entry: int = APP_ENTRY_POINT_DEFAULT,
) -> bytes:
    """
    Build signed image bytes (same as stm32_secure_boot ``sign_image.py``).
    Requires ``ecdsa`` and secp256k1 PEM private key.
    """
    import hashlib

    from ecdsa import SECP256k1, SigningKey
    from ecdsa.util import sigencode_string

    sk = SigningKey.from_pem(privkey_pem)
    if sk.curve != SECP256k1:
        raise ValueError("private key must be secp256k1")
    image_size = len(app)
    digest = hashlib.sha256(app).digest()
    raw_sig = sk.sign_digest_deterministic(
        digest,
        hashfunc=hashlib.sha256,
        sigencode=sigencode_string,
    )
    if len(raw_sig) != 64:
        raise ValueError("expected 64-byte raw signature")
    header = struct.pack(
        "<IIII",
        IMAGE_HEADER_MAGIC,
        1,  # version
        image_size,
        app_entry,
    )
    header += b"\x00" * 16
    header += raw_sig
    return header + app


def parse_signed_image(data: bytes) -> SignedImageParts:
    if len(data) < HEADER_SIZE:
        raise ValueError(f"too short: {len(data)} < {HEADER_SIZE}")
    magic, version, image_size, entry = struct.unpack_from("<IIII", data, 0)
    sig = data[32:HEADER_SIZE]
    app = data[HEADER_SIZE:]
    if len(sig) != 64:
        raise ValueError("bad signature length")
    return SignedImageParts(
        magic=magic,
        version=version,
        image_size=image_size,
        entry=entry,
        signature=sig,
        app=app,
    )


def verify_signed_image_ecdsa(parts: SignedImageParts, pubkey_pem: bytes) -> bool:
    """Return True if SHA-256(app) verifies with embedded secp256k1 signature."""
    try:
        from ecdsa import BadSignatureError, SECP256k1, VerifyingKey
        from ecdsa.util import sigdecode_string
    except ImportError:
        print("ERROR: pip install ecdsa", file=sys.stderr)
        return False

    if parts.magic != IMAGE_HEADER_MAGIC:
        return False
    if parts.image_size != len(parts.app):
        return False

    digest = hashlib.sha256(parts.app).digest()
    vk = VerifyingKey.from_pem(pubkey_pem)
    if vk.curve != SECP256k1:
        return False
    try:
        vk.verify_digest(parts.signature, digest, sigdecode=sigdecode_string)
        return True
    except BadSignatureError:
        return False
    except Exception:
        return False


def main() -> int:
    p = argparse.ArgumentParser(description="Verify stm32_secure_boot signed .bin (host)")
    p.add_argument("signed_bin", type=Path, help="Output of sign_image.py")
    p.add_argument(
        "--privkey-pem",
        type=Path,
        metavar="PEM",
        help="secp256k1 private key PEM (as in sign_image.py); public key is derived for verify",
    )
    args = p.parse_args()
    data = args.signed_bin.read_bytes()
    try:
        parts = parse_signed_image(data)
    except ValueError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 2

    pem_path = args.privkey_pem
    if pem_path is None:
        # default: try repo sibling
        here = Path(__file__).resolve().parent
        cand = here.parent.parent / "stm32_secure_boot" / "scripts" / "root_private_key.pem"
        if not cand.is_file():
            print(
                "ERROR: pass --privkey-pem or place stm32_secure_boot next to CryptoWallet",
                file=sys.stderr,
            )
            return 2
        pem_path = cand

    from ecdsa import SECP256k1, SigningKey  # noqa: PLC0415

    sk = SigningKey.from_pem(pem_path.read_bytes())
    if sk.curve != SECP256k1:
        print("ERROR: not secp256k1", file=sys.stderr)
        return 2
    vk_pem = sk.get_verifying_key().to_pem()

    ok = verify_signed_image_ecdsa(parts, vk_pem)
    print(f"magic=0x{parts.magic:08X} version={parts.version} app_len={len(parts.app)} ok={ok}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
