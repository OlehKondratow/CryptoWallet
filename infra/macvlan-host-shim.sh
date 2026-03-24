#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Access Gitea from LOCAL host via http://192.168.127.5:3000/ on macvlan network.
#
# Problem: Linux does not route traffic from host to a macvlan container IP on the same
# NIC, so 192.168.127.5 is not reachable from this same machine.
#
# Solution: create an extra macvlan interface on the host (same parent as compose),
# assign a free address in 192.168.126.0/23 - then host sees 192.168.127.5 as an L2 neighbor.
#
# Requires: root (sudo). Parent interface should match docker-compose (eno1).
#
# Usage:
#   sudo ./infra/macvlan-host-shim.sh add     # create interface and address
#   sudo ./infra/macvlan-host-shim.sh del     # remove it
#   ./infra/macvlan-host-shim.sh status       # without root: show whether shim exists
#
# After add, open: http://192.168.127.5:3000/
# Set in infra/.env:
#   GITEA_ROOT_URL=http://192.168.127.5:3000/
#   GITEA_SERVER_DOMAIN=192.168.127.5
# and run: podman compose -f infra/docker-compose.yml up -d
# -----------------------------------------------------------------------------
set -euo pipefail

PARENT="${GITEA_MACVLAN_PARENT:-eno1}"
# Free address in your /23 (must not conflict with container 192.168.127.5 and gateway)
SHIM_IP="${MACVLAN_HOST_SHIM_IP:-192.168.127.254/23}"
IFACE="${MACVLAN_HOST_SHIM_IFACE:-gitea-macvlan-shim}"

cmd_add() {
  if [[ "$(id -u)" -ne 0 ]]; then
    echo "Run: sudo $0 add"
    exit 1
  fi
  if ip link show "$IFACE" &>/dev/null; then
    echo "Interface $IFACE already exists."
    ip -4 addr show dev "$IFACE" || true
    exit 0
  fi
  if ! ip link show "$PARENT" &>/dev/null; then
    echo "Interface $PARENT not found. Set: export GITEA_MACVLAN_PARENT=<your_nic>"
    exit 1
  fi
  echo "Creating $IFACE on $PARENT with $SHIM_IP ..."
  ip link add "$IFACE" link "$PARENT" type macvlan mode bridge
  ip addr add "$SHIM_IP" dev "$IFACE"
  ip link set "$IFACE" up
  echo "Done. Check: ping -c1 192.168.127.5"
  ping -c1 -W2 192.168.127.5 && echo "OK: host can reach Gitea container."
}

cmd_del() {
  if [[ "$(id -u)" -ne 0 ]]; then
    echo "Run: sudo $0 del"
    exit 1
  fi
  if ip link show "$IFACE" &>/dev/null; then
    ip link del "$IFACE"
    echo "Removed: $IFACE"
  else
    echo "Interface $IFACE not found."
  fi
}

cmd_status() {
  if ip link show "$IFACE" &>/dev/null; then
    echo "Shim $IFACE: present"
    ip -4 addr show dev "$IFACE" 2>/dev/null || true
  else
    echo "Shim $IFACE: absent (sudo $0 add - to reach 192.168.127.5 from host)"
  fi
}

case "${1:-}" in
  add)  cmd_add ;;
  del)  cmd_del ;;
  status|"") cmd_status ;;
  *)
    echo "Usage: $0 add|del|status"
    exit 1
    ;;
esac
