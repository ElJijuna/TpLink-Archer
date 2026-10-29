# Etapa71 — coordinador multicanal probado sobre ARM

2026-10-29. Avance50 %. El flujo de recuperación ahora puede terminar en un
barrido pasivo6/11/1 y un informe conjunto validado por replay.

## Implementación

`tools/passive_workflow.py` admite `--sweep-1-6-11` para los tres modos
existentes. Conserva el escaneo inicial de canal1, necesario para establecer
el contexto antes del retune. Después ejecuta la herramienta nativa de etapa68,
exige su terminal de retorno con TX de red pausado/RXDCK limpio y genera
`sweep-report.json` mediante etapa70. Mantiene el bloqueo del flujo, conserva
binarios/logs/hashes y exige contexto CLEAN antes de completar.

El presupuesto pasa de22 a28 secuencias cuando se solicita barrido: seis
adicionales para PRE_NOTIFY/RXDCK en6/11/1. Sin presupuesto no lanza hijos;
no resetea épocas ni reintenta fases. El modo original de canal1 se conserva.
La salida `network_entries` sigue contando el escaneo inicial; los conteos
multicanal están en `workflow.json:sweep_report` y `sweep-report.json`.

```
python3 tools/passive_workflow.py cold-start artifacts/NEW_RUN --sweep-1-6-11
```

La selección cold-start presupone el estado de investigación compatible;
no es un detector universal del estado de cualquier adaptador.

## Prueba física

La inspección previa confirmó USB37ad:0104 y controles firmware status3,
run_environment0, BBMCU sin boot-ready y resets sin liberar. El flujo
`cold-start` terminó con código0 en `artifacts/passive-workflow-stage71`:
inspect, boot, radio, initial_rfk, tx_rfk, scan y sweep completados.

| Canal recibido en barrido | Entradas | Gestión aceptada | FCS erróneos |
|---|---:|---:|---:|
|1|15|79|0|
|6|13|137|0|
|11|21|137|0|

El informe conjunto contiene **49 BSSID distintos /49 pares BSSID-SSID**.
Cada canal coincide con el decoder offline y el escaneo inicial también pasa
su replay. El ejecutor confirmó retorno a canal1 y RXDCK limpio por destino.
Secuencias216→188, consumo28; reserva de contexto CLEAN. El escaneo inicial
tuvo25 entradas: no se suman como redes adicionales al barrido.

Evidencia local: `artifacts/passive-workflow-stage71/workflow.json`,
`sweep-report.json`, logs por fase y capturas por canal. No se afirma
readback independiente de versión de firmware ni calidad RF completa.
TX de red permaneció pausado; no hay asociación ni interfaz de red macOS.

## Validación y siguiente paso

`make test` pasa:69 tests Python más suites C++ existentes. Se añaden pruebas
de presupuesto28 antes de lanzar hijos y de terminal multicanal con rechazo
de TX no pausado, RXDCK incierto o promoción indebida de calidad RF.
Log `artifacts/passive-workflow-tests-stage71.log`.

Solo hardware ARM; Intel y Linux siguen pendientes. Esta etapa integra
capacidades ya puntuadas y no aumenta el50 %. El siguiente hito funcional
requiere avanzar TX y asociación, además de validar las semánticas/calidad
RF pendientes antes de una conexión de producción.
