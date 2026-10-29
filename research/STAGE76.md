# Etapa76 — contexto recuperado; RFK posterior detenido

2026-10-29. Mac ARM. Avance50 %. Sin nuevos envíos de tramas de red.

## Recuperación física

Se añadió a `tools/sequence_epoch.py` la opción explícita
`--discard-dirty-context`. Toma bloqueo exclusivo del archivo de contexto,
archiva sus bytes y solo permite pasar DIRTY→CLEAN después del reset
MAC/DLE, reset CPU, recarga completa de firmware sin H2C y drenaje250ms
verificados por el helper existente. Comprueba inode y estado antes de
confirmar. Un fallo conserva contexto/ledger anteriores y fence DIRTY.
El modo predeterminado sigue rechazando contexto DIRTY sin lanzar hijos.

```
python3 tools/sequence_epoch.py artifacts/NEW_RECOVERY --discard-dirty-context
```

Ejecución real en `artifacts/context-recovery-stage76`: committed,
contexto anterior DIRTY archivado, contexto nuevo CLEAN, secuencias170→254
en época nueva. El ledger anterior no se reutiliza: está archivado y el
cambio de época depende de la frontera de firmware verificada.
El marcador de prueba TX permanece `TAG1 RESERVED`; no se habilitó otro envío.

## Restauración parcial posterior

Se ejecutó el coordinador resume-bootstrap con barrido solicitado en
`artifacts/post-recovery-workflow-stage76`. Inspect y radio terminaron0.
initial_rfk envió únicamente PRE_NOTIFY (comando8, secuencia4), pero no
recibió terminal dentro del plazo. El coordinador salió1 y se detuvo antes
de DACK, calibración TX, captura y barrido. No se afirma recuperación completa
del escaneo ni validación RF de este estado.

Un drenaje pasivo posterior de2s (`--drain-only`) tampoco encontró terminal;
no se reenvió PRE_NOTIFY. Quedan251 secuencias. La lectura física posterior
`artifacts/tx-paused-stage76.jsonl` confirmó **0x10398=0**. El archivo de
contexto CLEAN describe el reset MAC previo; no certifica el RFK posterior.
La causa del terminal ausente sigue pendiente, sin atribuirla solo al plazo
o al transporte USB síncrono.

## Interpretación de los reportes de etapa75

mac.c:5848-5853 trata los reportes fallidos como progreso hasta que el contador
alcanza el límite solicitado; TX_DONE termina inmediatamente. Por tanto,
los contadores1/2/3/4 no deben clasificarse automáticamente como duplicados.
Se añadió `probe_report_terminal` y se actualizó el receptor a esa regla.
Para el límite1 configurado en etapa75, el primer reporte sigue siendo
terminal fallido: **no hay TX exitoso**. La razón por la que continuaron
contadores posteriores no queda resuelta por esta regla del host.

Los cambios de descriptor de etapa75 y esta corrección del receptor solo
se recompilaron; no se enviaron al hardware porque el marcador lo bloquea.
No se borró el marcador, se cambió a otro tag ni se repitió la trama.

## Validación

Build universal correcto. `make test` pasa:78 tests Python y suites C++.
Cobertura nueva: recuperación DIRTY solo tras frontera completa, fallo de
recarga que conserva DIRTY, rechazo predeterminado antes de lanzar hijos y
distinción de progreso/terminal TX para límites1/8. Log
`artifacts/context-recovery-tests-stage76-final.log`.

Siguiente trabajo: diagnosticar el PRE_NOTIFY sin terminal para recuperar
un perfil RF conocido; después resolver los intentos de TX antes de una
segunda prueba de red. Intel/Linux y asociación siguen pendientes.
