# Etapa85 — probe dirigido a Especial en canal9: TX_DONE, sin respuesta dirigida

2026-10-31. Mac ARM. Avance52 %. Una sola nueva entrega de red por USB.

Tras la calibración9 de etapa84 se preparó y ejecutó una solicitud de sondeo
con SSID IE `Especial`. Conserva DA/BSSID broadcast y las cuatro tasas CCK
 del descriptor anterior; la selección del objetivo se hace por SSID.
El cuerpo crece de32 a40 bytes:104 bytes totales con descriptor, longitud
actualizada, secuencia Wi-Fi2 y etiqueta de reporte2, distintas del intento1.

`tools/special_probe_attempt.py` exige el informe84 con evidencia íntegra,
ledger exacto de esa época (0..31 reservadas), contexto y frontera CLEAN.
Reserva un marcador nuevo y permanente para el intento9 antes de lanzar el
binario; un fallo o interrupción no permite repetirlo. El marcador histórico
 del intento1 permanece sin cambios. Los tests verifican rechazo de canal,
historia y etiqueta incorrectos, y bloqueo tras fallo del proceso hijo.
La herramienta nativa exige el descriptor heredado de esa reserva, valida
canal9 RF/BB y las197 escrituras residentes de potencia antes del envío.

## Resultado físico

- Una entrega endpoint05,104 bytes,status0.
- Un reporte TX_DONE con tag2,state0,count1.
- Nueve comandos de contexto, secuencias32..40, limpieza confirmada.
- Captura y replay del escaneo idénticos.
- Contexto CLEAN, TX de red pausado, callbacks drenados,215 secuencias libres.
- El adaptador sigue en canal9; no hubo asociación ni interfaz de red.

La auditoría offline `archer-special-probe-audit` verifica FCS por software,
SSID Especial, BSSID50926a110666, DS9 y dirección de destino de las respuestas:

| Observación | Cantidad |
|---|---:|
| Tramas de gestión aceptadas |310|
| FCS erróneos |0|
| Beacons del objetivo |31|
| Probe responses del objetivo |13|
| Probe responses a nuestra MAC |0|
| Probe responses del objetivo a nuestra MAC |0|

Los13 probe responses no se atribuyen a este sondeo: ninguno está dirigido
a02:00:00:89:22:a1. TX_DONE es evidencia del firmware, todavía no evidencia
externa de emisión/recepción correcta por el AP. La ausencia de respuesta
no demuestra por sí sola fallo RF. Persisten los límites DPK/TSSI de etapa84.

## Código y evidencia

Nuevos modos `make special-probe-universal` y
`make special-probe-audit-universal`; el modo anterior de canal1 conserva su
paquete y tag1. Tests de bytes del SSID/longitud/secuencia/tag y separación
 de reportes. `make test`:87 tests Python y suites C++ correctos. Compilan
universal las variantes normal, probe1, probe9 y auditor; Intel sin prueba física.

[special-probe-stage85-observation.json](special-probe-stage85-observation.json)
conserva hashes de capturas, binario, trama exacta, auditoría, logs y fuentes.
Artefactos locales en `artifacts/special-probe-stage85/`. El marcador nuevo
se conserva; no borrarlo para repetir el envío.

Siguiente: contrastar la emisión con un receptor independiente o auditar la
ruta TX/descriptor con el driver fuente. No ampliar a asociación mientras
no exista evidencia externa de respuesta a nuestra MAC.
