# Etapa59 — estado PHYINFO/CMAC/dispatcher

Fecha: 2026-10-22. Se añadieron lecturas de estado por fase, basadas en el
dump de errores de `mac_be.c:3094-3160`, revisión fijada
`c03c1a14507dfaf8b856c2608f9ac216021fc222`. No se limpian flags ni se
deduce su antigüedad a partir de nombres de registro.

## Preflight corregido

La primera ejecución se detuvo al leer100b8=DEADBEEF. La fuente solo usa
COMMON_PHYINTF_CTRL_0 en la rama tmac de RTL8922D, no en RTL8922A.
Se sustituyó por RCR11400 en el perfil de lecturas, sin inventar una escritura.
Antes de liberar la reserva DIRTY se verificaron0 eventos context_send,
0 firmware_bulk_write y0 ficheros TX. No se enviaron H2C ni se devolvieron
las nueve secuencias reservadas. Evidencia:
[context-stage59-preflight-release.json](context-stage59-preflight-release.json).

El ejecutor ahora libera automáticamente una reserva propia que no haya
llegado al primer intento de envío. El indicador se activa **antes** de
invocar send; una excepción de envío conserva DIRTY aunque la entrega sea
ambigua. Una reserva DIRTY previa sigue bloqueando antes de crear esa guarda.
No permite reciclar secuencias ni limpiar reservas de otros ciclos inciertos.

## Observación física del perfil corregido

Dieciséis muestras antes/después de configuraciones y comandos:

| Estado | Valor observado | Campos de la fuente |
|---|---|---|
| RMAC RX_ERR_ISR114f4 | 59hex | bits0/3/4/6, etapa57 |
| PHYINFO_ERR_ISR110fc | 0ehex | CCK_CCA, OFDM_CCA, DATA_ON timeout |
| CMAC_ERR_ISR10164 | 40hex | WMAC_RX_ERR_IND |
| DISP_ERROR_ISR08804 | c0hex | HDR_DMA_TIMEOUT_ERR, HDR_RX_TIMEOUT_ERR |
| DISP_ERROR_ISR1/2 | 0/0 | sin bits observados |

Todos esos flags estaban en la primera muestra y permanecieron iguales;
no se observaron bits nuevos respecto a ella. **No demuestra que no se
repitan eventos en un bit ya activo**, ni fecha el origen de estos flags.
Fuentes de campos: `reg.h:5142-5172,6711-6723,7838-7845,8305-8316`.
No se expande el significado de la sigla HDR más allá de esos símbolos.

Loopback MAC desactivado, interfaz PHY habilitada, DROP_NONDMA_PPDU0,
HAXI modo4/USB con RXDMA habilitado y master STOP0 en todas las muestras.
El registro de parada HAXI b010 conserva80000000 (STOP_WPDMA declarado en
la fuente); no se reinterpretó como parada RX ni se alteró sin una secuencia
de fuente que lo justifique. MPDU_PROC=30309. Estos controles no prueban
por sí solos que el recorrido de paquetes normales esté operativo.

Durante la captura de cinco segundos: CRC OK PHY CCK403, OFDM95 y HT7;
PLE grupo0 vacío y cuota6 usada0 en las muestras; ocho ACK de control,
**0 paquetes normales**. Contexto retirado, reserva CLEAN según su política,
TX de red pausado y stderr del perfil corregido vacío.

## Validación y siguiente contraste

49 suites C++ y52 tests Python. Dos tests nuevos verifican bits nuevos frente
al primer snapshot, flags persistentes, habilitaciones y rechazo de estados
malformados/inaccesibles. Binario corregido conservado antes de ejecutarse.
Evidencia completa y hashes:
[phy-mac-stage59-observation.json](phy-mac-stage59-observation.json).

Los indicadores priorizan el recorrido de recepción normal y su entrega DMA,
pero no identifican aún el punto que descarta o retiene paquetes. El siguiente
contraste debe incluir el comportamiento de lecturas bulk con timeout/transferencia
parcial y la ruta de recepción del driver USB, además de la configuración MAC.
No alterar flags o STOP_WPDMA por analogía con otro chip. Avance40%.
