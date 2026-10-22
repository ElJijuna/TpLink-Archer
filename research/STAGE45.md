# Etapa45 — ventana RX en dispositivo

Fecha: 2026-10-21. `archer-passive-rx --receive-channel1 NEW_DIRECTORY`
comprueba firmware cargado, ADDA, DMAC, APPEND_FCS, canal digital1 de ambas
rutas y CMAC0 con scheduler TX pausado. Toma el lock del ledger sin consumir
secuencias H2C. No envía comandos firmware ni cambia el canal.

La ventana diagnóstica no reproduce el ciclo completo de escaneo Linux.
Desactiva CMAC0 RESP_PKTCTL_EN, rechaza control/datos, admite únicamente
subtipos beacon/probe-response y configura recepción broadcast sin CAM ni
comprobación de beacon. No habilita sniffer ni aceptación de errores.
Conserva UID, longitud máxima y bits ajenos a la máscara. Restaura los cinco
campos en orden inverso con readback, incluso si falla una escritura o el
callback. Un fallo de restauración se comunica como error explícito.

Fuentes fijadas a `c03c1a14507dfaf8b856c2608f9ac216021fc222`:
`reg.h` R_BE_RX_FLTR_OPT/CTRL_FLTR/MGNT_FLTR/DATA_FLTR y RESP_PKTCTL_EN;
`mac_be.c:1280-1345` filtros BE de un bit por subtipo;
`core.c:5075-5076` eliminación de restricciones CAM durante ROC.
La combinación es una decisión de diagnóstico, no una secuencia de escaneo
validada por el fabricante ni una prueba independiente de silencio RF absoluto.

Prueba física ARM, cinco segundos, dos endpoints IN: **0 transferencias**.
No hubo beacons ni respuestas de sondeo observados. No se deduce ausencia de
redes ni fallo de sensibilidad a partir de este resultado.

Readback antes/después:

| Registro | Antes | Durante | Restaurado |
|---|---|---|---|
| CMAC0 FUNC 0x10000 | f00001ff | f000017f | f00001ff |
| CTRL 0x11424 | ffff | 0 | ffff |
| DATA 0x1142c | ffff | 0 | ffff |
| MGNT 0x11428 | ffff | 120 | ffff |
| RX OPT 0x11420 | 0f1740be | 0f170004 | 0f1740be |
| Scheduler 0x10398 | 0 | 0 | 0 |

Evidencia local en `artifacts/passive-rx-stage45.jsonl`, stderr vacío;
binario conservado `artifacts/archer-passive-rx-stage45`.
Hashes: [passive-rx-stage45-observation.json](passive-rx-stage45-observation.json).

Validación:37 suites C++ y46 tests Python; compilación universal ARM/Intel.
Las pruebas nuevas cubren restauración normal, excepción del callback,
fallos tras cada escritura y rechazo sin escrituras si TX está activo.
Registro: `artifacts/tests-stage45-rx.txt`.

Sigue pendiente investigar el recorrido RX BB/RMAC/DLE/USB con contadores
antes/después para localizar dónde se detiene. No se elevó el progreso40%:
la ventana está probada, la recepción de tramas todavía no.
