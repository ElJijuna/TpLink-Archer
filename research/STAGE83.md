# Etapa83 — calibración9 preparada; ejecución bloqueada por USB

2026-10-31. Avance52 %. **No se completó aún la calibración física de canal9.**

El ejecutable universal acepta ahora
`--owned-slot0-channel9-world-calibrate NEW_DIRECTORY`. Valida el arranque
sobre canal1, el scheduler pausado y los selectores MCC0. Aplica los parámetros
BB/RF de9 y su tabla de potencia separada antes de PRE_NOTIFY, TSSI scan,
PRE_NOTIFY, TXGAPK, IQK, TSSI normal, DPK y RXDCK. El parámetro de canal se
propaga a los comandos y a las comprobaciones de IQK/DPK/RXDCK. TSSI usa los
grupos CCK/OFDM3 y trim1 de canal9; los tests contrastan los bytes DE enviados
con valores sintéticos distintos para cada grupo y camino.

Una reserva CLEAN pasa a DIRTY antes de modificar la configuración. Solo se
restablece CLEAN después de todos los reportes, lectura final de2452MHz/RF9,
scheduler pausado y cierre/drenaje USB. El éxito deja el adaptador en canal9,
MCC0, sin interfaz/asociación ni nueva trama de gestión. Las herramientas que
exigen canal1 deben rechazar ese estado; no asumir retorno automático.
DPK/TSSI conservan sus límites de interpretación y calidad RF anteriores.

## Resultado físico de esta etapa

Dos aperturas de recepción fallaron antes de escribir registros o enviar H2C.
La segunda conservó el error preciso: endpoint0x84, `-536854449` / `0xe000404f`,
`kIOUSBPipeStalled` según USB.h del SDK local. Una lectura independiente de
identidad en0xfc falló con transferencia abortada, `-536870163`,0 bytes.
La enumeración USB todavía mostraba37ad:0104; eso no demuestra acceso funcional.

No se consumieron secuencias RFK ni se llegó a las calibraciones. Contexto y
frontera de época continuaban CLEAN. Se pidió reconexión física para recuperar
el acceso, sin repetir comandos RFK ni forzar un reset de firmware a ciegas.

Logs locales:
- `artifacts/channel9-calibrate-stage83{,.jsonl,.stderr}`
- `artifacts/channel9-calibrate-stage83-open-diagnostic{,.jsonl,.stderr}`
- `artifacts/channel9-stage83-identity.{jsonl,stderr}`

Build universal correcto. `make test` pasa:85 tests Python y suites C++,
incluida selección TSSI9 y campos DE. Logs en
`artifacts/channel9-calibrate-stage83-tests-final.log` y
`artifacts/channel9-calibrate-stage83-build-final.log`.

Siguiente: tras reconexión, identificar/expulsar el instalador si aparece,
recuperar una época fría verificable y radio inicial, luego ejecutar la opción
nueva en una carpeta distinta. No marcar esta tarea completada hasta disponer
de los terminales/reportes físicos del canal9.
