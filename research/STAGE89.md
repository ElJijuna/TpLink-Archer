# Etapa89 — sondeo unicast medido: actividad TX interna, sin respuesta confirmada

2026-10-31. Mac ARM. Avance52 %. Una nueva solicitud de sondeo, no asociación.

## Contraste e instrumentación

Tras CLEAN87 y baseline88 se preparó un único probe a Especial/BSSID
50926a110666/canal9, con tag4 y secuencia Wi-Fi4. Mantiene40 bytes de trama
(104 con descriptor), CCK1 y las cuatro tasas básicas del AP.
Se desactiva explícitamente RTS_EN respecto al unicast86:
info4=0x80000000 en lugar de0x88000000; HW_RTS_EN permanece1.
Es una desviación diagnóstica deliberada del default unicast de Linux.

El coordinador `--monitored` exige la calibración84 y sus capturas íntegras,
la finalización CLEAN87, el ledger posterior idéntico (0..49 reservadas),
baseline88 y sus hashes. Reserva un marcador permanente nuevo para tag4,
conservando todos los anteriores. El nativo verifica canal/potencia residentes.

En contexto listo se mide una baseline2000ms y, después, otra ventana2000ms
que incluye una sola entrega del probe. Ambas exigen DONE nuevo y restauración
 de controles. La baseline TX debe ser0 antes de admitir el envío.

## Resultado físico

| Ventana | Lectura tras ms | TX unidades32us | Tiempo TX según contador |
|---|---:|---:|---:|
|baseline|2002|0|0us|
|con un probe|2009|17|544us|

Una entrega endpoint05,104 bytes,status0. El firmware emitió un reporte
**tag4,state1=RETRY_LIMIT,count1**. No hubo terminal de éxito.
La actividad interna durante la ventana con un probe contrasta con baseline0;
no prueba potencia RF correcta, emisión por antena ni recepción del AP.
Tampoco asigna semánticas nuevas a DPK/TSSI.

La auditoría offline obtuvo146 tramas de gestión aceptadas yFCS0. Del AP
objetivo:62 beacons,8 probe responses;0 respuestas dirigidas a nuestra MAC.
El flag RTS desactivado no resolvió el fallo en este experimento. No se
identifica todavía una causa única entre calidad TX/RF, recepción/control
 de respuesta y condiciones del AP.

## Limpieza y error TX independientes

Las nueve operaciones de contexto usaron secuencias50..58. La retirada terminó
confirmada; el scheduler volvió a0, las ventanas/restauración se verificaron y
los callbacks drenaron antes de CLEAN. El helper sigue devolviendo error2 por
el fallo TX, y el coordinador conserva estado stopped y su marcador; no
convierte una limpieza correcta en éxito de transmisión.

Se restringió el tratamiento de fallo conocido: solo el error TX terminal
explícito se difiere hasta después de la limpieza. Un error de monitor,
restauración, transporte o reporte ausente no entra en esa excepción.

**Final: canal9, MCC0, contexto CLEAN, TX pausado,197 secuencias libres.
Una sola nueva trama; ningún reset de firmware ni reintento automático.**

## Evidencia y validación

[monitored-probe-stage89-observation.json](monitored-probe-stage89-observation.json)
conserva resultados, estado y hashes de fuente, binario, trama, capturas y logs.
Artefactos locales en `artifacts/special-monitored-probe-stage89/`.
`make test`:91 tests Python y suites C++, incluidas ventanas ordenadas,
baseline0, restauración, bytes/flags/tag4. Compilación universal de las variantes
normal/probe1/probe9/unicast9/medida correcta; Intel sin prueba física.

Siguiente: contrastar por aire con otro receptor o instrumentar evidencia
adicional documentada de TX/recepción de ACK. La medición interna ha reducido
la incertidumbre de actividad TX, pero aún no confirma comunicación con el AP.
