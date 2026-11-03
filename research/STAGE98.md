# Etapa98 — validaciones repetidas, independientes y CRC software

Fecha: 2026-11-03. Sin nuevo TX ni recalibración.

## Resultados físicos y efectos de la auditoría

Se ejecutaron tres snapshots consecutivos. Los210 registros de ruta/potencia
coincidieron entre ejecuciones y con el plan del host. Se tomaron tres lecturas
pasivas de contadores PHY por ejecución, separadas150ms: habilitados y sin
variaciones. No se hizo reset de estadísticas durante la auditoría.

El validador offline comprueba todos los transfers USB, anchos/completed_bytes,
status0, unicidad/completitud de210 registros, terminal de finalización,
restauración de la ventana WLAN y ausencia de escrituras inesperadas. Solo
aparecen apertura/cierre WLAN0x74 y tres solicitudes XTAL-SI de lectura de
identidad0x270 (0x41/fd/fe); no hay programación de potencia. Los hashes de
ledger, época, contexto y todos los marcadores de sondeos son idénticos antes
y después. Contexto CLEAN y197 secuencias.

La vía independiente compara las siete palabras de ruta AB directamente con
el bloque de referencia rtw8922a.c, y las cuatro tasas CCK con las celdas del
elemento9 del firmware ligado por hash. Ambas coinciden, incluida palabra
CCK0x28282828. Esto no es una validación independiente de todos los210 valores
ni demuestra que la inicialización completa sea suficiente para emitir.

## Captura externa reanalizada mediante CRC32

El auditor monitor_crc_audit.py comprueba el contenedor classic PCAP,
radiotap y sus límites, presencia de FCS y CRC32 software. Sobre el PCAP del
tag6:2477 registros a2452MHz;1463 CRC válidos,1010 inválidos y4 paquetes
truncados excluidos del CRC. No hay discrepancias entre CRC y flag bad-fcs
en los registros auditables. Se confirman381 beacons válidos de Especial,
35 ACK,151 CTS y0 tramas válidas con direcciones de la MAC de prueba.
Esto refuerza la observación negativa, sin demostrar ausencia de energía RF.
El perfil con padding radiotap no se audita; queda contado como no disponible.

## Corrección e instrumentación

Se corrigió STAGE97: RETRY_LIMIT terminal sale antes del reset posterior de
contadores, por lo que ese reset no explica los0 observados tras tag6.
No hay baseline emparejado de aquel envío.

Ahora el ejecutor toma CCK_TXEN/TXON y OFDM_TXEN/TXON antes del envío y
después de pausar TX, antes de evaluar el terminal o resetear contadores.
Rechaza controles deshabilitados. Publica delta módulo65536, que no distingue
wrap, saturación ni reset intermedio; no afirma emisión por antena. La nueva
instrumentación compila, pero aún no se ejecutó con TX. No habilita repetir
ningún marcador consumido ni modifica los límites de admisión existentes.

## Validación de software y límites

Suites C++ y103 tests Python aprobados. Nuevos casos: extracción de campos,
wrap, controles inválidos, fallo de lectura en cada registro sin escrituras,
transfers cortos, mutación inesperada/XTAL opcode incorrecto, ventana sin
restaurar, registros duplicados/faltantes, contadores pasivos cambiantes,
CRC corrupto, SSID equivocado, PCAP truncado y tipo de enlace incorrecto.
Compilaciones universales: auditor TX, context-rx, sondeo externo, ACK y
monitored. No se ha probado hardware Intel. Avance ponderado52 %.

Evidencia y hashes: [tx-validation-stage98-observation.json](tx-validation-stage98-observation.json).
