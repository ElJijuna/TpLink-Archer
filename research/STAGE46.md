# Etapa46 — recepción PHY confirmada por contadores

Fecha: 2026-10-21. Se añadieron snapshots antes/después al ejecutor RX y
una opción explícita `--receive-channel1-reset-counters` que descarta las
estadísticas PMAC previas. No reinicia radio, BB o firmware; no retunea.

La primera medición sin reset encontró varios contadores saturados y sin
cambio. No permitía inferir ausencia de actividad. HCI_FUNC_EN=7,
RX_STOP=0, DISP_FWD_WLAN_0=55; las banderas RMAC 10c00/10c84 eran0.
La combinación no prueba que todas las colas o filtros funcionen.

Se aplicó el reset PMAC de `rtw89_phy_pmac_stat_reset` en el Linux fijado
`c03c1a14507dfaf8b856c2608f9ac216021fc222`, `phy.c:5963-5981`,
con el mapa `rtw8922a_pmac_regs` de `rtw8922a.c:380-425` y apertura BB+20000.
CCK R1B_RX_RPT_RST bit7:0→1; RST_ALL_CNT bit0:1→0.
ENABLE_ALL_CNT bit1 ya estaba1 y se exige ese estado. Los controles vuelven
a sus campos iniciales aun si falla una escritura; el historial de contadores
se pierde deliberadamente y no se presenta como reversible.

Medición física ARM de cinco segundos, después del reset:

| Contador | Tras reset | Al finalizar |
|---|---|---|
| OFDM CCA 20e24 | 0 | 000002a9 |
| CCK CCA 22e10 | 00000005 | 002c1c4b |
| CCK CRC 22e14 | 0 | 00ee0178 |
| Legacy OFDM CRC 20e64 | 0 | 001f001f |
| HT CRC 20e60 | 0 | 00060005 |
| VHT CRC 20e5c | 0 | 0 |
| HE CRC 20e58 | 0 | 0 |

Los campos CRC OK son bits15:0 y error bits31:16 según el mapa fuente:
CCK376 OK/238 error, OFDM31 OK/31 error, HT5 OK/6 error. Confirman actividad
de decodificación PHY con CRC correcto. No identifican beacons, SSID, BSSID,
integridad de datos entregados al host ni calidad/sensibilidad RF suficiente.
El valor CCK CCA completo se conserva sin interpretar sus bits fuera de máscara.

**USB siguió en0 transferencias.** El bloqueo queda acotado a una etapa
posterior a la decodificación PHY: filtros/entrega RMAC, DLE, despacho o USB.
No se ha aislado cuál de ellas. No se afirma que PHY→RMAC funcione.
Scheduler0, filtros y motor de respuestas restaurados; stderr vacío.

Validación:38 suites C++ y46 tests Python, compilación universal ARM/Intel.
Los tests nuevos cubren guardas sin escrituras y limpieza después de fallar
cada paso del reset. Binario stage46b conservado; hashes y snapshots en
[rx-counters-stage46-observation.json](rx-counters-stage46-observation.json).
Registro: `artifacts/tests-stage46-final.txt`.

Próximo diagnóstico: ocupación de colas PLE/RMAC y ruta al host, conservando
la distinción entre tramas decodificadas por PHY y tramas recibidas por USB.
Avance ponderado40%: aún no se completa la ruta RX del controlador.
