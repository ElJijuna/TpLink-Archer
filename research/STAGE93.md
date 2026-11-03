# Etapa93 — preparación de receptor independiente

Fecha: 2026-11-02.

networksetup identificó la Wi-Fi integrada como en0. Están disponibles
tcpdump4.99.1 (Apple161) y libpcap1.10.1. wdutil no ofrece una orden de
sniffer en la ayuda de esta instalación.

La consulta de tipos de enlace en modo monitor quedó bloqueada por sudo:
se requiere autenticación interactiva del usuario. No se inició captura,
no se modificó el canal y no se transmitió otra trama del Archer.

El helper tools/inspect_monitor_capture.sh consulta únicamente los tipos de
enlace mediante tcpdump -i en0 -I -L. Debe ejecutarse con sudo en Terminal.
La apertura del adaptador en modo monitor puede afectar temporalmente Wi-Fi.

Siguiente condición: confirmar IEEE802_11_RADIO/radiotap y poder fijar canal9
antes de coordinar una captura acotada con un intento nuevo y marcador propio.
No reutilizar el intento tag5 de etapa92. Estado Archer no revalidado en esta
etapa; el último estado observado era CLEAN,206 secuencias y TX pausado.

## Capacidad confirmada y captura preparada

El usuario ejecutó el helper y confirmó IEEE802_11_RADIO. CoreWLAN consultado
sin cambios informó canal actual36 y soporte de canal9/20MHz. El Archer
superó --paused-channel9 sin H2C ni reset; esa comprobación abre/restaura la
ventana de registros y no envía trama de red.

make monitor-capture-universal construye un helper Intel/ARM. Su modo
--capture-channel9 NUEVO_DIRECTORIO exige sudo, desconecta en0 de su red,
selecciona9/20MHz y confirma el canal antes de lanzar tcpdump con radiotap.
Captura solo gestión/control durante un máximo aproximado de20 segundos,
interrumpe tcpdump, restaura el canal original y solicita reconectar la red
si macOS no lo hace. No guarda contraseñas ni realiza asociación automática.
Rechaza directorios existentes y no envía tráfico con el Archer.

La consulta --inspect está validada físicamente; la captura y restauración
aún requieren ejecución con autenticación del usuario. Después debe revisarse
la frecuencia radiotap y la presencia del AP del canal9: el readback inicial
no demuestra que el receptor permanezca en ese canal durante toda la captura.
No se enviará un nuevo sondeo hasta validar ese baseline independiente.
