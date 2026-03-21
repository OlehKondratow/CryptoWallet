#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Доступ с ЛОКАЛЬНОГО хоста к Gitea по http://192.168.127.5:3000/ при сети macvlan.
#
# Проблема: Linux не маршрутизирует трафик с хоста на IP контейнера macvlan на том же
# NIC — поэтому 192.168.127.5 «не пингуется» с этой же машины.
#
# Решение: поднять на хосте дополнительный интерфейс macvlan (тот же parent, что у
# compose), выдать ему свободный адрес в 192.168.126.0/23 — после этого хост видит
# 192.168.127.5 как соседа в L2.
#
# Требуется: root (sudo). Родительский интерфейс — как в docker-compose (eno1).
#
# Использование:
#   sudo ./infra/macvlan-host-shim.sh add     # создать интерфейс и адрес
#   sudo ./infra/macvlan-host-shim.sh del     # удалить
#   ./infra/macvlan-host-shim.sh status       # без root: показать, есть ли shim
#
# После add откройте: http://192.168.127.5:3000/
# В infra/.env задайте:
#   GITEA_ROOT_URL=http://192.168.127.5:3000/
#   GITEA_SERVER_DOMAIN=192.168.127.5
# и: podman compose -f infra/docker-compose.yml up -d
# -----------------------------------------------------------------------------
set -euo pipefail

PARENT="${GITEA_MACVLAN_PARENT:-eno1}"
# Свободный адрес в вашей /23 (не должен совпадать с контейнером 192.168.127.5 и шлюзом)
SHIM_IP="${MACVLAN_HOST_SHIM_IP:-192.168.127.254/23}"
IFACE="${MACVLAN_HOST_SHIM_IFACE:-gitea-macvlan-shim}"

cmd_add() {
  if [[ "$(id -u)" -ne 0 ]]; then
    echo "Запустите: sudo $0 add"
    exit 1
  fi
  if ip link show "$IFACE" &>/dev/null; then
    echo "Интерфейс $IFACE уже существует."
    ip -4 addr show dev "$IFACE" || true
    exit 0
  fi
  if ! ip link show "$PARENT" &>/dev/null; then
    echo "Нет интерфейса $PARENT. Задайте: export GITEA_MACVLAN_PARENT=ваш_nic"
    exit 1
  fi
  echo "Создаю $IFACE на $PARENT с $SHIM_IP ..."
  ip link add "$IFACE" link "$PARENT" type macvlan mode bridge
  ip addr add "$SHIM_IP" dev "$IFACE"
  ip link set "$IFACE" up
  echo "Готово. Проверка: ping -c1 192.168.127.5"
  ping -c1 -W2 192.168.127.5 && echo "OK: хост видит контейнер Gitea."
}

cmd_del() {
  if [[ "$(id -u)" -ne 0 ]]; then
    echo "Запустите: sudo $0 del"
    exit 1
  fi
  if ip link show "$IFACE" &>/dev/null; then
    ip link del "$IFACE"
    echo "Удалено: $IFACE"
  else
    echo "Интерфейс $IFACE не найден."
  fi
}

cmd_status() {
  if ip link show "$IFACE" &>/dev/null; then
    echo "Shim $IFACE: есть"
    ip -4 addr show dev "$IFACE" 2>/dev/null || true
  else
    echo "Shim $IFACE: нет (sudo $0 add — чтобы ходить на 192.168.127.5 с хоста)"
  fi
}

case "${1:-}" in
  add)  cmd_add ;;
  del)  cmd_del ;;
  status|"") cmd_status ;;
  *)
    echo "Использование: $0 add|del|status"
    exit 1
    ;;
esac
