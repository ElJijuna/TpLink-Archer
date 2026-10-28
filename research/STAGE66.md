# Etapa66 — transición explícita de época y reanudación comprobadas

Fecha: 2026-10-27. Se archivó la época anterior, se recargó firmware mediante
un camino sin H2C y se publicó un ledger nuevo. Después se reanudó radio,
calibración y escaneo con16 entradas y417 observaciones válidas.

## Frontera de firmware

`src/macos_epoch_boot.cpp` es un helper de coordinador para el perfil cargado
y alimentado conocido. Exige los descriptores heredados del workflow y del
ledger anterior, comprueba su identidad, y exige una marca durable DIRTY.
No instancia SequenceLedger ni reserva/envía H2C.

Comprueba chip/AID, alimentación y scheduler pausado. Con USB sin lecturas
pendientes ejecuta reset_to_usb2_scc: detiene productores, verifica resets y
reinicializa DLE. Después ejecuta enable_adda_power y boot_firmware con los
dos blobs fijados. Conserva la omisión experimental DCPU de etapas anteriores.
El perfil descarga212 transferencias FWDL (193 principal y19 BBMCU0), con
éxito de transporte y verificación de controles/estado de inicialización.
Estos frames FWDL no son comandos H2C de contexto/radio/calibración.

Tras cerrar el canal de descarga, abre recepción asíncrona y descarta/retiene
cualquier residuo hasta observar250 ms sin transferencias, con límite de3 s.
Abort/drain termina todas las peticiones antes de emitir el resultado terminal.
En la prueba física se descartaron0 transferencias. La pausa observada no se
presenta como prueba aislada contra cualquier ACK tardío: la frontera se basa
conjuntamente en ownership, reset verificado, nueva descarga/arranque y ausencia
de H2C posteriores. No hay lectura independiente de versión residente.

El helper se detiene justo después del boot, con los controles necesarios para
`radio-init --resume-loaded-scc-radio`. No envía las tablas de radio antes del
cambio de ledger. Así no hay secuencias H2C de la época nueva que deban copiarse
desde el bitmap antiguo. Si en el futuro se añaden H2C aquí, esta premisa cambia
y sus reservas deberán conservarse en el ledger nuevo.

## Transacción del ledger

`tools/sequence_epoch.py` exige contexto CLEAN y no acepta una transición previa
incompleta. Toma lock exclusivo del workflow y del ledger antiguo. Conserva una
copia exacta y fsync en previous-ledger.bin, junto a hashes y UUID de época.

Antes de tocar hardware publica `.epoch-transition=DIRTY`. Las herramientas
actualizadas que usan SequenceLedger rechazan DIRTY, marcas malformadas o
symlinks, incluso después de que termine el proceso coordinador. El helper
especializado es la única fase de esta operación que no necesita SequenceLedger;
hereda los dos locks y comprueba la marca DIRTY.

Solo con salida0 y la secuencia terminal requerida se admite el cambio. El
validador exige reset MAC verificado, cpu_reset, firmware_ready,212 envíos FWDL
exitosos, callbacks pendientes0, periodo quieto y h2c_submissions0, en orden.
También verifica que el ledger anterior no cambió ni se sustituyó su inode.

El archivo nuevo se escribe/fsync y bloquea antes de replace, con fsync del
directorio. El lock antiguo continúa activo hasta terminar. Después persiste
el checkpoint ledger_replaced y publica CLEAN. El ledger conserva su formato
ARCHSEQ1 y su prohibición de wrap;0/1 siguen reservados conservadoramente.
El UUID y el historial de firmware están en el manifiesto externo.

Un fallo de recarga conserva el ledger anterior y DIRTY; no habilita reintentos.
Un corte entre replace y CLEAN deja el ledger nuevo bloqueado para revisión,
sin rollback automático. Un corte posterior a CLEAN puede dejar el manifiesto
en ledger_replaced, pero la frontera verificada ya está persistida. No se
implementa recuperación automática de esos estados interrumpidos. Binarios
históricos sin el protocolo nuevo no deben mezclarse con estas herramientas.

## Prueba física completa sobre Mac ARM

1. `python3 tools/sequence_epoch.py artifacts/epoch-stage66`: salida0, committed.
   Archivo anterior íntegro con11 secuencias libres; archivo nuevo con254.
2. `python3 tools/passive_workflow.py resume-bootstrap artifacts/workflow-stage66`:
   salida0. Inspección → radio sin repetir reset/boot → RFK → TX RFK → scan/replay.
3. Consumo22 secuencias en la época nueva; quedan232. Ledger anterior archivado,
   contexto CLEAN y fence CLEAN.

Escaneo:451 normales,417 observaciones de gestión aceptadas,16 entradas,
bad_fcs0, unsupported34 y overflow0. El resultado en vivo coincide con el replay
offline. TX de red permanece pausado y los callbacks USB se retiraron. Calidad
DPK/TSSI sigue sin validación completa; no se eleva a éxito por esta transición.
Los conteos abarcan todas las fases RX y el canal permanece fijo en1.

## Validación y alcance

52 suites C++ y64 tests Python pasan. Se amplían pruebas de SequenceLedger
para marcas DIRTY/CLEAN/symlink y pruebas de transacción con helpers simulados:
recarga fallida sin publicación ni pérdida del archivo anterior, frontera
incompleta/desordenada/H2C no cero/callbacks pendientes, y commit correcto con
archivo anterior preservado. Una prueba adicional rechaza una marca symlink
colgante antes de lanzar procesos. Esa guarda de preflight se añadió después
de la prueba física y se verificó en la suite.

El modo resume-bootstrap debe usarse inmediatamente después de una transición
committed del historial controlado. Sus comprobaciones nativas verifican el
estado de boot esperado; no descubren de forma general la identidad de un
firmware desconocido. La variante cold-start del coordinador sigue sin una
prueba física automatizada específica; no se confunde con esta reanudación.

La transición es explícita, no un reset del ledger al agotarse ni un permiso
para reutilizar secuencias dentro de una época. Su validez física se ha probado
una vez en RTL8922A CV1/USB2/SCC sobre ARM; no en Intel/Linux ni frente a
cortes de energía físicos. El avance funcional sigue en48 %.

El próximo objetivo es ampliar el barrido pasivo de canales mediante el
coordinador, con transición de radio y restauración verificadas.

Evidencia y hashes: [epoch-stage66-observation.json](epoch-stage66-observation.json).
