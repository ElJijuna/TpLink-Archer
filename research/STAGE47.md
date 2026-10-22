# Etapa47 — observación PLE y prueba RMAC CH_EN

Fecha: 2026-10-21. Se añadió un lector diagnóstico PLE limitado a QEMPTY
grupo0, cuota6 y QLNKTBL selector1 para colas no vacías del grupo0.
No asigna, libera ni desencola buffers. Rechaza la interfaz ocupada,
espera como máximo1000 lecturas/2ms de plazo host, sin reintento, y restaura
el selector previo después de completarse. Si la petición queda activa,
la deja intacta y devuelve error; no intenta cancelarla con una escritura.

Fuente Linux fijada `c03c1a14507dfaf8b856c2608f9ac216021fc222`:
`mac.c:118-196`, `mac.h:127-134,714-718`, `reg.h` registros PLE9110/9114,
máscaras de páginas reservadas/usadas bits11:0/27:16. La fuente usa polling
1us/1000us; el ejecutor admite2ms de plazo host por el coste de USB macOS,
conservando el límite de1000 lecturas. El snapshot no es atómico ni continuo.

## Observación física

Dos ventanas de cinco segundos, ambas con reset diagnóstico de contadores,
TX scheduler pausado y motor de respuestas desactivado durante recepción:

| Observación | Perfil anterior | CH_EN diagnóstico |
|---|---|---|
| RCR 11400 durante ventana | 20f3e000 | 20f3e001 |
| PLE grupo0 vacío, las tres muestras | ffffffff | ffffffff |
| Cuota6, reservado/usado | 57/0 | 57/0 |
| CCK CRC OK final | 450 | 262 |
| OFDM CRC OK final | 39 | 26 |
| HT CRC OK final | 5 | 0 |
| Transferencias USB | 0 | 0 |

RCR B_BE_CH_EN está declarado como bit0 en `reg.h:8156`. La inicialización
BE de la fuente no lo activa explícitamente; no se traslada la escritura
AX a producción por analogía. `--diagnostic-rmac-channel1` lo activa solo
dentro de la ventana reversible, como hipótesis aislada. Readback verificó
0→1→0. La prueba **no demuestra que CH_EN sea una habilitación necesaria**
en RTL8922A ni que su estado anterior causara el bloqueo. Activarlo no bastó
para entregar paquetes. El perfil predeterminado mantiene su comportamiento.

La ausencia de ocupación PLE en tres instantes no prueba que nunca entrara
un paquete ni cubre todos los grupos. Cuota6 mide páginas usadas en esos
instantes, no un contador acumulado. La evidencia sigue acotando el problema
a una etapa posterior a PHY, sin distinguir todavía filtrado, entrega RMAC,
despacho/DLE o salida USB.

## Validación y siguiente paso

39 suites C++ y46 tests Python; ejecutor compila ARM/Intel. Tests nuevos
cubren interfaz ocupada sin escrituras, selector inválido, finalización,
restauración tras fallo de lectura y petición pendiente sin segunda escritura.
La ventana verifica restauración del sexto registro tras un fallo de escritura.

Evidencia y hashes en [ple-rmac-stage47-observation.json](ple-rmac-stage47-observation.json).
Binario stage47b conservado; ambos stderr vacíos. Próximo contraste: reglas
de aceptación de beacon/A1/BSSID y entrega PHY→RMAC, con variación de una
condición por prueba. El avance ponderado continúa40%.
