# Etapa84 — secuencia de calibración física completada en canal9

2026-10-31, Mac ARM, Archer37ad:0104. Avance52 %: el hito RF completo sigue
pendiente de interpretación/medición de calidad, aunque la secuencia9 ya terminó.

Tras reconexión y expulsión autenticada del CD virtual, el cambio directo
produjo sense/error sin CSW válido. La enumeración posterior confirmó modo
Wi-Fi37ad:0104. La frontera fría verificada descargó firmware sin H2C,
drenó USB y creó254 secuencias. El bootstrap frío completó radio,
calibraciones1 y captura de25 entradas con replay idéntico; dejó232 secuencias.

Después se ejecutó el binario conservado con
`--owned-slot0-channel9-world-calibrate`. Aplicó BB/RF9 y las197 escrituras
 del perfil de potencia9, manteniendo pausado el scheduler de red.

| Operación | Secuencia | Resultado |
|---|---:|---|
| PRE_NOTIFY9 |24| terminal OK |
| TSSI scan9 |25| terminal OK |
| PRE_NOTIFY9 |26| terminal OK |
| TXGAPK9 |27| terminal OK, ok1, sin failcodes |
| IQK9 |28| terminal OK, flags limpios, canal coincide |
| TSSI normal9 |29| terminal OK, alineación [0,0,0,0] |
| DPK9 |30| terminal OK, errores limpios, canal coincide, estados [4,4] |
| RXDCK9 |31| terminal OK, ambos caminos9, timeouts [0,0] |

El replay offline independiente de las capturas confirmó exactamente un
reporte RXDCK limpio para9/20MHz/2G. Se comprobaron ocho entregas H2C
correctas y ocho terminales; no hubo envío de nueva trama de red.
Las lecturas finales confirmaron2452MHz y ambos registros RF digitales9.
RF18 analógico del caminoB sigue leyendo0, como en el perfil observado de1;
el sintetizador seleccionadoA y ambos registros digitales sí coincidieron.

El cierre abortó/drenó todos los callbacks USB antes de devolver la reserva
a CLEAN. **Estado final: canal9, MCC0, TX pausado, contexto CLEAN,
224 secuencias disponibles.** No se volvió a1. Las herramientas que exigen1
no deben usarse sobre este estado sin una transición explícita.

Esto completa la ejecución/reportes de la secuencia, no la validación de
calidad RF: DPK [4,4] y TSSI [0,0,0,0] conservan sus semánticas pendientes.
Sin prueba de respuesta del AP, asociación, contraseña ni interfaz de red.

## Evidencia

[channel9-calibration-stage84-observation.json](channel9-calibration-stage84-observation.json)
conserva hashes de binario, capturas, replay y fuentes, además de los resultados.
Logs locales en `artifacts/channel9-calibration-stage84-evidence/`;
recuperación en `artifacts/channel9-cold-recovery-stage84/` y bootstrap en
`artifacts/channel9-bootstrap-stage84/`. La compilación universal y `make test`
(85 tests Python y suites C++) pasaron en etapa83, sin cambios posteriores
al código ejecutado. Intel aún sin prueba física.

Siguiente: preparar intento único de probe dirigido a Especial en9 con
reserva de reporte sin reutilización, conservar el estado de canal9 y
capturar una respuesta externa. La calibración por sí sola no autoriza
borrar el marcador histórico ni repetir el probe anterior.
