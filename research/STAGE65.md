# Etapa65 — coordinador reproducible de recuperación y escaneo

Fecha: 2026-10-27. `tools/passive_workflow.py` ejecuta y documenta las fases
nativas del perfil experimental. El recorrido recover-loaded se probó completo
sobre Mac ARM, con salida0 y14 entradas de escaneo.

## Recorrido probado

El adaptador seguía en37ad:0104 con el firmware/radio del historial controlado.
El coordinador congeló los binarios actuales en su directorio de intento y
completó inspect → radio → initial_rfk → tx_rfk → scan → replay offline.
Cada herramienta conserva sus propias comprobaciones de chip, alimentación,
firmware, radio y ownership; no se sustituye un preflight fallido por una
escritura correctiva ni se cambia automáticamente de modo.

El escaneo produjo264 paquetes normales,250 observaciones de gestión aceptadas
y14 entradas; bad_fcs0, unsupported14 y overflow0. El agregado en vivo coincide
exactamente con el replay offline. El retiro de contexto fue reconocido, TX de
red quedó pausado, la reserva terminó CLEAN y los callbacks USB drenaron.
La calibración conserva quality_validated=false y los desconocidos DPK/TSSI.

## Ownership entre procesos

`WorkflowLock` acompaña a cada SequenceLedger en las herramientas recompiladas.
Una herramienta independiente obtiene bloqueo compartido en el archivo
`artifacts/37ad-0104-sequences.bin.workflow`, antes del bloqueo habitual del
ledger. El coordinador obtiene bloqueo exclusivo para todo el recorrido y
transfiere el descriptor mediante pass_fds/ARCHER_WORKFLOW_FD a sus hijos.
Los hijos comprueban inode/dispositivo del archivo y retienen un duplicado de
la misma descripción abierta. No liberan el bloqueo exclusivo del padre al salir.

Esto impide intercalar herramientas actualizadas entre fases. Las copias de
binarios históricos anteriores al cambio no implementan este protocolo; no se
debe mezclarlas con el coordinador. No es un lock impuesto a otros drivers o
procesos externos al proyecto. La prueba física de todos los hijos bajo el
lock exclusivo confirma que la herencia funciona en este recorrido.

## Persistencia y errores

Se exige un directorio nuevo, contexto CLEAN y22 secuencias disponibles antes
de tocar USB. Se conservan stdout/stderr por fase, binarios y hashes. El
manifiesto workflow.json registra intención antes de lanzar cada hijo y resultado
verificado después. Se actualiza con archivo temporal, fsync, replace y fsync
del directorio. Una fase con exit0 también debe emitir su terminal esperado;
scan debe confirmar TX pausado y drenaje USB.

No se reintenta ni se salta una fase incierta. Un error deja status=stopped y
no avanza. No hay reinicio automático del ledger ni de reservas DIRTY. Un
manifiesto interrupted/started no autoriza repetir comandos. No se añade un
timeout externo que mate al ejecutor durante su cleanup; se usan sus esperas
nativas acotadas. Un SIGKILL/corte de energía puede dejar el último estado
persistido en started/running; no significa éxito ni cleanup completado.

Los modos son explícitos:

- recover-loaded: requiere el perfil cargado conocido y usa
  radio-init --powered-cold-scc-radio. Fue probado físicamente en esta etapa.
- cold-start: inspección, carga inicial --boot-usb-no-dcpu y después
  radio-init --cold-scc-radio, RFK y escaneo. Reutiliza el recorrido manual
  validado en etapa63, pero **esta variante coordinada no se probó físicamente**.
  No expulsa el CD virtual ni realiza el cambio de modo por sí sola.

La lectura inspect valida controles; no demuestra la versión residente de
firmware. El historial de carga sigue siendo parte del perfil de investigación.

## Uso

```
make test firmware-state-universal firmware-boot-universal radio-init-universal rfk-universal tx-calibrate-universal context-rx-universal passive-scan-decode-universal
python3 tools/passive_workflow.py recover-loaded artifacts/NEW_WORKFLOW
```

Los binarios nativos necesitan el acceso USB habitual. Deben recompilarse antes
del recorrido para incorporar el protocolo de ownership. El comando no es un
instalador ni un driver de red y todavía mantiene el escaneo fijo en canal1.

## Presupuesto y validación

El ciclo consumió exactamente22 secuencias:33 disponibles antes y11 después.
El ledger sigue íntegro y no se reinició al recargar firmware. El coordinador
rechazará otro ciclo completo antes de lanzar procesos. Hace falta diseñar una
transición de época vinculada a un reinicio comprobado y sin comandos pendientes;
no basta borrar el archivo o permitir wrap. Esto precede a más ciclos completos.

52 suites C++ y60 tests Python pasan. Los nuevos tests cubren exclusión entre
herramientas, descriptor heredado correcto/incorrecto, retención del lock del
padre, terminales ambiguos, fallo de hijo sin siguiente fase y rechazo por
presupuesto insuficiente sin lanzar procesos. Build universal correcto;
validación física solo ARM.

El avance funcional se mantiene en48 %: se ha hecho reproducible un recorrido
existente, sin añadir TX, asociación, DriverKit ni barrido de canales.

Manifiesto físico: `artifacts/workflow-stage65/workflow.json`.
Hashes y resumen: [workflow-stage65-observation.json](workflow-stage65-observation.json).
