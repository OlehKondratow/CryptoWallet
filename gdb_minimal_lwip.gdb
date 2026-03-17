# GDB script for debugging minimal-lwip
# Usage: cd /data/projects/CryptoWallet && arm-none-eabi-gdb -x gdb_minimal_lwip.gdb
# Prerequisites: st-util running (listens on :4242)

# Load ELF first — required for symbols (bt) and load command
file build/cryptowallet.elf
directory .

# Map libgcc sources to Drivers/ (avoids "lib1funcs.S: No such file")
set substitute-path /build/gcc-arm-none-eabi-13.2.1/libgcc/config/arm /data/projects/CryptoWallet/Drivers/libgcc/config/arm
set substitute-path /build/gcc-13.2.1/libgcc/config/arm /data/projects/CryptoWallet/Drivers/libgcc/config/arm

target extended-remote :4242

monitor reset halt
load

# Breakpoints: net_task (entry), tcpip_init (where hang likely occurs)
break net_task
break tcpip_init

# Run. If break at net_task: type c to continue.
# When UART shows "Net: start" and hangs: Ctrl+C, then: bt
continue
