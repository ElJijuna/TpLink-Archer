# Etapa81 — Especial confirmado en canal9 con el Archer

2026-10-30. Mac ARM. Avance52 %. Recepción pasiva1→9→1 probada físicamente.

## Objetivo identificado

El Archer recibió el SSID **Especial** en canal9 y el AP anunció DS9.
La captura contiene26 beacons y2 probe responses de ese BSSID, todos
aceptados con FCS verificado. Los probe responses se observaron pasivamente;
no se atribuyen a un envío del Archer en esta etapa. Esto confirma por fin
el nombre y canal que el resultado CoreWLAN ocultaba en etapa80.

| Canal recibido | Entradas | Gestión aceptada | FCS erróneos | Replay |
|---|---:|---:|---:|---|
|9|12|134|0|idéntico|
|1, retorno|15|74|0|idéntico|

RXDCK limpio en ambos destinos, retorno a2412MHz/canal1 verificado,
scheduler TX pausado y reserva CLEAN. Quedan228 secuencias. El marcador
de la prueba de gestión sigue conservado; **no hubo nueva trama de red TX**.

## Recuperación fría desde contexto DIRTY

El usuario reconectó el USB y expulsó el CD verificado. Una expulsión directa
dio una respuesta sense y después error, sin CSW válido. La enumeración
posterior confirmó37ad:0104; los controles BB eran los de arranque frío.

Se añadieron al helper/coordinador las rutas `--reload-cold-no-h2c` y
`--cold-after-reconnect`. Exigen el perfil frío y archivan contexto/ledger;
resetean CPU y cargan firmware sin H2C antes de permitir una época nueva.
El modo posterior `cold-radio-bootstrap` reproduce la inicialización RF
desde la alimentación fría: omite una descarga inicial duplicada del
coordinador, pero radio-init conserva su reset/recarga propios, como en el
flujo frío validado de etapa78.

La primera comprobación final del helper comparó el perfil frío con una
expectativa incorrecta (7 bajo máscara0x300007). La descarga sí había llegado
a firmware_ready; el valor observado era0x300007, distinto del perfil caliente2.
Se corrigió la expectativa. **No se repitió la descarga**: la ruta separada
`--finish-cold-boundary` verificó controles actuales y drenó USB, combinando
esa evidencia con las212 entregas y fases conservadas del intento inicial.
Solo entonces se comprometieron ledger nuevo y contexto CLEAN.

La finalización exige log original íntegro, contexto/ledger sin cambios,
fence DIRTY y los locks originales; no convierte una descarga incompleta en
éxito. Un fallo conserva el bloqueo. Los logs/binarios iniciales y de
finalización están preservados en `artifacts/special-cold-recovery-stage81/`.
La recuperación abrió254 secuencias; el bootstrap consumió22 y el retune4.

## Prueba de radio y canal9

`artifacts/special-radio-workflow-stage81` completó inspect, radio, RFK,
calibración y captura inicial en1 (23 entradas). Después se ejecutó:

```
./build/archer-passive-retune-universal --owned-slot0-channel1-9-1 artifacts/special-channel9-stage81
./build/archer-passive-scan-decode-universal --channel9-fcs-appended artifacts/special-channel9-stage81/channel-9
```

Los dos dwells coinciden exactamente con su replay offline. No se amplió
TX ni su tabla de potencia a9. Las semánticas/calidad DPK/TSSI y el reset
en caliente conservan sus límites anteriores. Los resultados no prueban
todavía respuesta externa al TX de etapa79 ni asociación con Especial.

## Evidencia y validación

[target-channel9-stage81-observation.json](target-channel9-stage81-observation.json)
conserva hashes, conteos y coincidencia del objetivo. Logs/capturas en
`artifacts/special-channel9-stage81*`; recuperaciones y bootstrap con el
prefijo special de esta etapa. No se requirió contraseña de la red.

Build universal correcto; Intel sin prueba física. `make test` pasa:
84 tests Python y suites C++. Se añadieron tests de la frontera fría frente
a la caliente, bootstrap sin boot duplicado y finalización sin recarga,
incluido fallo que conserva ledger/contexto/fence bloqueados.
Log `artifacts/channel9-tests-stage81-final.log`.

Siguiente: preparar el perfil TX/calibración de canal9 y una prueba dirigida
a Especial, o mover temporalmente el AP a1. No enviar el descriptor de
canal1 sobre9 sin revalidar RF/potencia y reserva de intento.
