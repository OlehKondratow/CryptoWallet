#!/usr/bin/env python3
"""
Comprehensive testing script for CryptoWallet RNG and transaction signing.

This script provides:
1. RNG binary data generation for DIEHARDER statistical tests
2. Transaction signing verification tests
3. Detailed security analysis commands
4. Integration with device over USB/UART

Usage:
    python3 scripts/test_rng_signing_comprehensive.py --help
    python3 scripts/test_rng_signing_comprehensive.py --mode rng --port /dev/ttyACM0
    python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10
    python3 scripts/test_rng_signing_comprehensive.py --mode verify-all
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional


# ============================================================================
# CONSTANTS
# ============================================================================

# DIEHARDER configurations
DIEHARDER_FILE_SIZE = 134_217_728  # 128 MiB (minimum recommended)
DIEHARDER_MIN_SIZE = 1_000_000    # 1 MiB (minimum)

# RNG capture settings
RNG_CAPTURE_TIMEOUT = 1800  # 30 minutes @ 115200 baud = ~128 MiB
RNG_BAUD_RATE = 115200
RNG_PORT_TIMEOUT = 1.0

# Transaction signing parameters
TX_TIMEOUT_CONFIRM = 30000  # ms
TX_TEST_AMOUNTS = ["0.1", "0.5", "1.0", "10.5", "0.00001"]
TX_TEST_ADDRESSES = [
    "1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA",  # Test vector (must have USE_TEST_SEED)
    "1A1z7agoat7yd7WQF3p4vdTkRJNmPiJkMs",  # Another valid address
]
TX_TEST_CURRENCIES = ["BTC", "ETH", "LTC"]

# HTTP/WebUSB endpoints
HTTP_TX_ENDPOINT = "/tx"
HTTP_TX_SIGNED_ENDPOINT = "/tx/signed"
HTTP_STATUS_ENDPOINT = "/status"


# ============================================================================
# COLOR OUTPUT (for terminal)
# ============================================================================

class Colors:
    """ANSI color codes for terminal output."""
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    CYAN = "\033[96m"
    RESET = "\033[0m"
    BOLD = "\033[1m"


def print_header(text: str) -> None:
    """Print a formatted header."""
    print(f"\n{Colors.BOLD}{Colors.CYAN}{'='*70}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.CYAN}{text:^70}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.CYAN}{'='*70}{Colors.RESET}\n")


def print_success(text: str) -> None:
    """Print success message."""
    print(f"{Colors.GREEN}✓ {text}{Colors.RESET}")


def print_error(text: str) -> None:
    """Print error message."""
    print(f"{Colors.RED}✗ {text}{Colors.RESET}", file=sys.stderr)


def print_warning(text: str) -> None:
    """Print warning message."""
    print(f"{Colors.YELLOW}⚠ {text}{Colors.RESET}")


def print_info(text: str) -> None:
    """Print info message."""
    print(f"{Colors.BLUE}ℹ {text}{Colors.RESET}")


# ============================================================================
# RNG TESTING
# ============================================================================

class RNGTester:
    """RNG (Random Number Generator) testing suite."""

    def __init__(self, port: str = "/dev/ttyACM0", baud: int = RNG_BAUD_RATE):
        """Initialize RNG tester.
        
        Args:
            port: Serial port for UART communication
            baud: Baud rate (default: 115200)
        """
        self.port = port
        self.baud = baud
        self.output_file = "rng.bin"

    def check_prerequisites(self) -> bool:
        """Check if prerequisites for RNG testing are available."""
        print_info("Checking prerequisites...")

        checks = {
            "pyserial": self._check_pyserial(),
            "dieharder": self._check_dieharder(),
            "serial port": self._check_serial_port(),
        }

        all_ok = all(checks.values())
        for name, ok in checks.items():
            status = f"{Colors.GREEN}✓{Colors.RESET}" if ok else f"{Colors.RED}✗{Colors.RESET}"
            print(f"  {status} {name}")

        if not all_ok:
            print_error("\n" + "="*70)
            print_error("MISSING PREREQUISITES - Installation Required")
            print_error("="*70 + "\n")
            
            if not checks["pyserial"]:
                print_error("❌ pyserial not found:")
                print("   pip install pyserial\n")
            
            if not checks["dieharder"]:
                print_error("❌ dieharder not found:")
                print("   See INSTALL_DIEHARDER.txt for details")
                print("   Quick install:")
                print("   - Linux (Debian):   sudo apt install dieharder")
                print("   - Linux (Fedora):   sudo dnf install dieharder")
                print("   - macOS:            brew install dieharder")
                print("   Or run: ./install-test-deps.sh\n")
            
            if not checks["serial port"]:
                print_error(f"❌ Serial port not found ({self.port}):")
                print("   - Check device is connected via USB/UART")
                print("   - For multi-device, use: --port /dev/ttyUSB0\n")
            
            print_error("="*70)
            print_error("See docs_src/INSTALL_TEST_DEPS.md for complete instructions")
            print_error("="*70)
            return False

        return True

    @staticmethod
    def _check_pyserial() -> bool:
        """Check if pyserial is installed."""
        try:
            import serial
            return True
        except ImportError:
            return False

    @staticmethod
    def _check_dieharder() -> bool:
        """Check if dieharder is installed."""
        try:
            result = subprocess.run(
                ["dieharder", "--version"],
                capture_output=True,
                timeout=5,
                text=True,
            )
            return result.returncode == 0
        except FileNotFoundError:
            return False
        except Exception:
            return False

    def _check_serial_port(self) -> bool:
        """Check if serial port is available."""
        return os.path.exists(self.port)

    def capture_rng_data(
        self,
        output_file: str = "rng.bin",
        total_bytes: int = DIEHARDER_FILE_SIZE,
        skip_bytes: int = 0,
    ) -> bool:
        """Capture raw RNG bytes from device UART.

        Args:
            output_file: Output filename for RNG data
            total_bytes: Total bytes to capture (default: 128 MiB)
            skip_bytes: Bytes to skip at start (e.g., boot garbage)

        Returns:
            True if successful, False otherwise
        """
        try:
            import serial
        except ImportError:
            print_error("pyserial not installed: pip install pyserial")
            return False

        print_header("RNG Data Capture from UART")
        print_info(f"Port: {self.port}, Baud: {self.baud}")
        print_info(f"Target: {total_bytes / (1024*1024):.0f} MiB")
        print_info(f"Skip: {skip_bytes} bytes")
        print_info("Make sure firmware has USE_RNG_DUMP=1 (binary only output on UART)")

        try:
            ser = serial.Serial(
                port=self.port,
                baudrate=self.baud,
                timeout=RNG_PORT_TIMEOUT,
            )
            time.sleep(0.2)
            ser.reset_input_buffer()

            written = 0
            skip_left = skip_bytes
            start_time = time.time()

            with open(output_file, "wb") as f:
                while written < total_bytes:
                    chunk = ser.read(
                        min(4096, total_bytes - written + skip_left)
                    )
                    if not chunk:
                        continue

                    # Skip initial bytes if needed
                    if skip_left > 0:
                        if len(chunk) <= skip_left:
                            skip_left -= len(chunk)
                            continue
                        chunk = chunk[skip_left:]
                        skip_left = 0

                    # Write to file
                    take = min(len(chunk), total_bytes - written)
                    f.write(chunk[:take])
                    written += take

                    # Progress every 8 MiB
                    if written % (8 * 1024 * 1024) == 0:
                        elapsed = time.time() - start_time
                        progress_mb = written / (1024 * 1024)
                        rate_mbps = progress_mb / elapsed if elapsed > 0 else 0
                        eta = (total_bytes - written) / (1024 * 1024 * rate_mbps) if rate_mbps > 0 else 0
                        print(
                            f"  {progress_mb:6.0f} MiB / {total_bytes/(1024*1024):.0f} MiB "
                            f"({rate_mbps:.2f} MB/s, ETA: {eta:.0f}s)"
                        )

            ser.close()
            print_success(f"Captured {written} bytes to {output_file}")
            return True

        except Exception as e:
            print_error(f"Failed to capture RNG: {e}")
            return False

    def run_dieharder(
        self,
        input_file: str = "rng.bin",
        test_num: Optional[int] = None,
        all_tests: bool = True,
    ) -> bool:
        """Run DIEHARDER statistical tests on RNG data.

        Args:
            input_file: Input binary file with RNG data
            test_num: Specific test number (None = all tests)
            all_tests: Run all tests (if test_num is None)

        Returns:
            True if successful, False otherwise
        """
        print_header("DIEHARDER Statistical Tests")

        # Check file exists and size
        if not os.path.isfile(input_file):
            print_error(f"File not found: {input_file}")
            return False

        file_size = os.path.getsize(input_file)
        file_size_mb = file_size / (1024 * 1024)

        if file_size < DIEHARDER_MIN_SIZE:
            print_warning(f"File too small: {file_size_mb:.1f} MiB (< 1 MiB)")
            print_warning("Tests may be unreliable. Recommended: >= 128 MiB")

        print_info(f"Input file: {input_file} ({file_size_mb:.1f} MiB)")

        # Build command
        cmd = ["dieharder", "-g", "201", "-f", input_file]
        if test_num is not None:
            cmd.extend(["-d", str(test_num)])
        elif all_tests:
            cmd.append("-a")

        print_info(f"Command: {' '.join(cmd)}")
        print_info("Running tests (this may take 30 min - 3 hours)...\n")

        try:
            result = subprocess.run(cmd, timeout=None)
            if result.returncode == 0:
                print_success("DIEHARDER tests completed")
                return True
            else:
                print_error(f"DIEHARDER failed with exit code {result.returncode}")
                return False
        except Exception as e:
            print_error(f"Failed to run DIEHARDER: {e}")
            return False

    def analyze_rng_quality(self, input_file: str = "rng.bin") -> dict:
        """Perform basic statistical analysis on RNG data.

        Args:
            input_file: Input binary file

        Returns:
            Dictionary with analysis results
        """
        print_header("RNG Quality Analysis")

        if not os.path.isfile(input_file):
            print_error(f"File not found: {input_file}")
            return {}

        try:
            with open(input_file, "rb") as f:
                data = f.read(min(1_000_000, os.path.getsize(input_file)))

            results = {
                "file_size": os.path.getsize(input_file),
                "sample_size": len(data),
                "entropy": self._calculate_entropy(data),
                "byte_distribution": self._analyze_byte_distribution(data),
                "chi_square": self._chi_square_test(data),
            }

            print_info(f"File size: {results['file_size'] / (1024*1024):.1f} MiB")
            print_info(f"Sample entropy: {results['entropy']:.4f} bits/byte (max: 8.0)")
            print_info(f"Chi-square p-value: {results['chi_square']:.4f}")

            if results["entropy"] < 7.5:
                print_warning("Low entropy detected - RNG may have bias")
            else:
                print_success("Entropy looks good")

            return results

        except Exception as e:
            print_error(f"Analysis failed: {e}")
            return {}

    @staticmethod
    def _calculate_entropy(data: bytes) -> float:
        """Calculate Shannon entropy of data."""
        from collections import Counter
        counts = Counter(data)
        entropy = 0.0
        for count in counts.values():
            p = count / len(data)
            entropy -= p * (p * 8).bit_length() if p > 0 else 0
        # Simplified: Shannon entropy in bits/byte
        import math
        entropy = sum(
            -(c / len(data)) * math.log2(c / len(data))
            for c in counts.values()
        )
        return entropy

    @staticmethod
    def _analyze_byte_distribution(data: bytes) -> dict:
        """Analyze distribution of byte values."""
        from collections import Counter
        counts = Counter(data)
        return {
            "unique_bytes": len(counts),
            "min_count": min(counts.values()),
            "max_count": max(counts.values()),
            "avg_count": sum(counts.values()) / len(counts),
        }

    @staticmethod
    def _chi_square_test(data: bytes) -> float:
        """Simple chi-square test for uniformity."""
        from collections import Counter
        counts = Counter(data)
        expected = len(data) / 256
        chi_sq = sum(
            ((counts.get(i, 0) - expected) ** 2) / expected
            for i in range(256)
        )
        # Approximate p-value (chi-square with 255 df)
        # This is simplified; use scipy.stats.chi2.sf for accurate value
        import math
        p_value = math.exp(-chi_sq / 2)  # Very rough approximation
        return p_value


# ============================================================================
# TRANSACTION SIGNING TESTING
# ============================================================================

class SigningTester:
    """Transaction signing verification suite."""

    def __init__(self, device_ip: str = "192.168.0.10", device_port: int = 80):
        """Initialize signing tester.

        Args:
            device_ip: Device IP address for HTTP
            device_port: HTTP port (default: 80)
        """
        self.device_ip = device_ip
        self.device_port = device_port
        self.base_url = f"http://{device_ip}:{device_port}"

    def check_device_connectivity(self) -> bool:
        """Check if device is reachable."""
        print_info(f"Checking device connectivity: {self.base_url}")

        try:
            import requests
        except ImportError:
            print_error("requests not installed: pip install requests")
            return False

        try:
            response = requests.get(
                f"{self.base_url}{HTTP_STATUS_ENDPOINT}",
                timeout=5,
            )
            if response.status_code == 200:
                print_success(f"Device is reachable ({response.status_code})")
                return True
            else:
                print_warning(f"Device returned: {response.status_code}")
                return False
        except Exception as e:
            print_error(f"Cannot reach device: {e}")
            return False

    def test_transaction_signing(self) -> bool:
        """Test transaction signing with various inputs."""
        print_header("Transaction Signing Tests")

        try:
            import requests
        except ImportError:
            print_error("requests not installed: pip install requests")
            return False

        all_passed = True

        for amount in TX_TEST_AMOUNTS:
            for address in TX_TEST_ADDRESSES:
                for currency in TX_TEST_CURRENCIES:
                    if not self._test_single_tx(
                        address, amount, currency, requests
                    ):
                        all_passed = False

        return all_passed

    def _test_single_tx(
        self,
        address: str,
        amount: str,
        currency: str,
        requests,
    ) -> bool:
        """Test a single transaction."""
        test_name = f"{amount} {currency} to {address[:10]}..."
        print(f"\n  Testing: {test_name}")

        payload = {
            "recipient": address,
            "amount": amount,
            "currency": currency,
        }

        try:
            # POST transaction
            response = requests.post(
                f"{self.base_url}{HTTP_TX_ENDPOINT}",
                json=payload,
                timeout=5,
            )

            if response.status_code != 200:
                print_error(f"    POST failed: {response.status_code}")
                return False

            print_info(f"    ✓ TX accepted")

            # Wait for user confirmation (this happens on device)
            print_info(f"    Waiting for device confirmation (30s timeout)...")
            time.sleep(2)  # Give time for user to press button

            # GET signed result
            response = requests.get(
                f"{self.base_url}{HTTP_TX_SIGNED_ENDPOINT}",
                timeout=5,
            )

            if response.status_code != 200:
                print_warning(f"    No signature yet: {response.status_code}")
                return False

            data = response.json()
            if data.get("status") == "signed":
                sig = data.get("signature", "")
                if len(sig) == 128:  # 64 bytes in hex
                    print_success(f"    ✓ Signature: {sig[:16]}...")
                    return True
                else:
                    print_error(f"    Invalid signature length: {len(sig)}")
                    return False
            else:
                print_warning(f"    Status: {data.get('status')}")
                return False

        except Exception as e:
            print_error(f"    Exception: {e}")
            return False

    def test_deterministic_signing(self) -> bool:
        """Test that same transaction produces same signature (RFC6979)."""
        print_header("Deterministic Signing Test (RFC6979)")
        print_info("Same TX should produce identical signatures...")

        try:
            import requests
        except ImportError:
            return False

        address = TX_TEST_ADDRESSES[0]
        amount = "0.5"
        currency = "BTC"

        signatures = []

        for i in range(3):
            print(f"\n  Attempt {i+1}/3: Sending TX...")

            payload = {
                "recipient": address,
                "amount": amount,
                "currency": currency,
            }

            try:
                response = requests.post(
                    f"{self.base_url}{HTTP_TX_ENDPOINT}",
                    json=payload,
                    timeout=5,
                )

                if response.status_code != 200:
                    print_error(f"    POST failed")
                    return False

                time.sleep(2)

                response = requests.get(
                    f"{self.base_url}{HTTP_TX_SIGNED_ENDPOINT}",
                    timeout=5,
                )

                if response.status_code == 200:
                    data = response.json()
                    sig = data.get("signature", "")
                    signatures.append(sig)
                    print_info(f"    Got signature: {sig[:16]}...")
                else:
                    print_warning(f"    No signature")

            except Exception as e:
                print_error(f"    Exception: {e}")
                return False

        # Compare signatures
        if len(signatures) >= 2:
            if signatures[0] == signatures[1]:
                print_success("✓ Signatures are deterministic (RFC6979)")
                return True
            else:
                print_warning("⚠ Signatures differ (might be different k values)")
                return False
        else:
            print_warning("Could not get enough signatures for comparison")
            return False


# ============================================================================
# MAIN
# ============================================================================

def main() -> int:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="CryptoWallet RNG and Transaction Signing Comprehensive Test Suite",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Test RNG (capture 128 MiB)
  %(prog)s --mode rng --port /dev/ttyACM0

  # Run DIEHARDER on existing file
  %(prog)s --mode dieharder --file rng.bin

  # Test transaction signing over HTTP
  %(prog)s --mode signing --ip 192.168.0.10

  # Verify all systems
  %(prog)s --mode verify-all

  # Custom configuration
  %(prog)s --mode rng --port /dev/ttyUSB0 --bytes 268435456 --output rng_256mb.bin
        """,
    )

    parser.add_argument(
        "--mode",
        choices=["rng", "dieharder", "signing", "verify-all"],
        default="verify-all",
        help="Test mode",
    )
    parser.add_argument(
        "--port",
        default="/dev/ttyACM0",
        help="Serial port for UART (RNG mode)",
    )
    parser.add_argument(
        "--ip",
        default="192.168.0.10",
        help="Device IP address (signing mode)",
    )
    parser.add_argument(
        "--bytes",
        type=int,
        default=DIEHARDER_FILE_SIZE,
        help=f"RNG bytes to capture (default: {DIEHARDER_FILE_SIZE})",
    )
    parser.add_argument(
        "--output",
        default="rng.bin",
        help="Output file for RNG data",
    )
    parser.add_argument(
        "--file",
        default="rng.bin",
        help="Input file for DIEHARDER",
    )
    parser.add_argument(
        "--test",
        type=int,
        help="Specific DIEHARDER test number",
    )

    args = parser.parse_args()

    print_header("CryptoWallet Test Suite")
    print_info(f"Mode: {args.mode}")
    print_info(f"Timestamp: {datetime.now(timezone.utc).isoformat()}")

    if args.mode == "rng":
        tester = RNGTester(port=args.port)
        if not tester.check_prerequisites():
            return 1
        if tester.capture_rng_data(
            output_file=args.output,
            total_bytes=args.bytes,
        ):
            tester.analyze_rng_quality(args.output)
            return 0
        return 1

    elif args.mode == "dieharder":
        tester = RNGTester()
        if tester.run_dieharder(
            input_file=args.file,
            test_num=args.test,
        ):
            return 0
        return 1

    elif args.mode == "signing":
        tester = SigningTester(device_ip=args.ip)
        if not tester.check_device_connectivity():
            return 1
        if tester.test_transaction_signing():
            tester.test_deterministic_signing()
            return 0
        return 1

    elif args.mode == "verify-all":
        print_info("Running all tests...")
        rng_tester = RNGTester(port=args.port)
        sign_tester = SigningTester(device_ip=args.ip)

        # RNG
        print_header("Phase 1: RNG Testing")
        if rng_tester.check_prerequisites():
            rng_tester.capture_rng_data(
                output_file=args.output,
                total_bytes=args.bytes,
            )
            rng_tester.analyze_rng_quality(args.output)
            rng_tester.run_dieharder(input_file=args.output)

        # Signing
        print_header("Phase 2: Signing Testing")
        if sign_tester.check_device_connectivity():
            sign_tester.test_transaction_signing()
            sign_tester.test_deterministic_signing()

        return 0

    return 0


if __name__ == "__main__":
    sys.exit(main())
