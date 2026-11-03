#!/bin/sh
# Read-only capability query; does not start a packet capture or send Archer TX.
set -eu
if [ "$(id -u)" -ne 0 ]; then
    echo "Ejecuta este helper con sudo desde Terminal." >&2
    exit 1
fi
printf '%s\n' '=== Tipos de enlace disponibles en modo monitor para en0 ==='
/usr/sbin/tcpdump -i en0 -I -L
printf '%s\n' '=== Fin: no se inició captura ni se transmitió con el Archer ==='
