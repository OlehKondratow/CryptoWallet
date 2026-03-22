"""
Host tests: **stm32_secure_boot** image format (ECDSA secp256k1 over SHA-256(app)),
micro-tamper detection (app body, signature, header magic).

Uses the demo private key from sibling repo if present:
  ``../stm32_secure_boot/scripts/root_private_key.pem``

Set ``STM32_SECURE_BOOT`` to override the repo root.

**Note:** current tooling uses **PEM private key + ECDSA**, not X.509 certificates.
"""
from __future__ import annotations

import os
from pathlib import Path

import pytest

pytest.importorskip("ecdsa")

from ecdsa import SigningKey  # noqa: E402

from secure_boot_image import (  # noqa: E402
    IMAGE_HEADER_MAGIC,
    HEADER_SIZE,
    parse_signed_image,
    sign_app_bytes,
    verify_signed_image_ecdsa,
)


def _ssb_root() -> Path:
    env = os.environ.get("STM32_SECURE_BOOT", "").strip()
    if env:
        return Path(env)
    cw = Path(__file__).resolve().parents[2]
    return cw.parent / "stm32_secure_boot"


def _privkey_path() -> Path | None:
    p = _ssb_root() / "scripts" / "root_private_key.pem"
    return p if p.is_file() else None


@pytest.fixture(scope="module")
def privkey_pem() -> bytes:
    path = _privkey_path()
    if path is None:
        pytest.skip(
            "stm32_secure_boot/scripts/root_private_key.pem not found "
            "(clone sibling repo or set STM32_SECURE_BOOT)",
        )
    return path.read_bytes()


def test_sign_verify_roundtrip(privkey_pem: bytes) -> None:
    app = bytes(range(256)) * 2
    raw = sign_app_bytes(app, privkey_pem)
    parts = parse_signed_image(raw)
    sk = SigningKey.from_pem(privkey_pem)
    vk_pem = sk.get_verifying_key().to_pem()
    assert verify_signed_image_ecdsa(parts, vk_pem) is True


def test_tamper_one_app_byte_fails(privkey_pem: bytes) -> None:
    app = b"hello-app-payload-" * 32
    raw = bytearray(sign_app_bytes(app, privkey_pem))
    sk = SigningKey.from_pem(privkey_pem)
    vk_pem = sk.get_verifying_key().to_pem()
    # flip first byte of application region
    raw[HEADER_SIZE] ^= 0x55
    parts = parse_signed_image(bytes(raw))
    assert verify_signed_image_ecdsa(parts, vk_pem) is False


def test_tamper_signature_byte_fails(privkey_pem: bytes) -> None:
    app = b"x" * 500
    raw = bytearray(sign_app_bytes(app, privkey_pem))
    sk = SigningKey.from_pem(privkey_pem)
    vk_pem = sk.get_verifying_key().to_pem()
    raw[64] ^= 0x80  # signature bytes are raw[32:96)
    parts = parse_signed_image(bytes(raw))
    assert verify_signed_image_ecdsa(parts, vk_pem) is False


def test_wrong_magic_fails(privkey_pem: bytes) -> None:
    app = b"a" * 100
    raw = bytearray(sign_app_bytes(app, privkey_pem))
    struct_pack = __import__("struct").pack
    raw[0:4] = struct_pack("<I", 0xDEADBEEF)
    parts = parse_signed_image(bytes(raw))
    sk = SigningKey.from_pem(privkey_pem)
    vk_pem = sk.get_verifying_key().to_pem()
    assert parts.magic != IMAGE_HEADER_MAGIC
    assert verify_signed_image_ecdsa(parts, vk_pem) is False
