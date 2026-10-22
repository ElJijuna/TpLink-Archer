# Etapa43 — metadatos RX

Fecha: 2026-10-21. Se añadió `rx_descriptor.hpp`, integrado en `archer-decode`,
con campos derivados de las fuentes fijadas y límites descritos en
[RX_DESCRIPTOR.md](RX_DESCRIPTOR.md).

Observación offline: `artifacts/rfk-stage33/rx-0.bin` produce un paquete tipo0,
descriptor largo40, payload124, CRC_ERR=false, ICV_ERR=false, tasa cruda260,
BW crudo0 y HDR_CNV=false. Los indicadores del descriptor no sustituyen una
verificación independiente de FCS ni validan contenido 802.11. No se atribuye
canal, SSID o recepción actual a esta captura histórica.

La política FCS permanece desconocida en el CLI offline porque el fichero
por sí solo no demuestra la configuración MAC durante su adquisición.
No se ejecutaron escrituras USB ni nuevas calibraciones en esta etapa.

Validación: `make test decode decode-universal` terminó con código0;
35 suites C++ y46 tests Python. Registro local:
`artifacts/tests-stage43-rx.txt`. `git diff --check` limpio.

Pendiente: ventana de filtros RX reversible para recepción pasiva con CMAC
actual, parser acotado de beacon/probe-response y captura reproducible.
DPK/TSSI conservan los límites de etapa42. El progreso ponderado permanece40%.
