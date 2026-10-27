# Etapa63 — recepción normal reproducida después de recuperar el arranque

Fecha: 2026-10-27. Dos ejecuciones del perfil USB2/SCC reciben **555 paquetes
normales, con555 FCS verificados**, incluidos448 beacons y77 probe responses.
TX de red permaneció pausado. No hay todavía asociación ni interfaz de red.

## Auditoría PHY añadida

Se leen ocho controles de PHY0, canal1/2.4GHz/AB, antes de ventanas, con
contexto listo y después de restaurarlas. No se modifican estos controles.

| Control | Dirección física | Máscara | Esperado bajo máscara |
|---|---|---|---|
| Reset PHY liberado |20704hex|2hex|2hex|
| CCK CCA habilitada |20520hex|1hex|0|
| CCK habilitado |20700hex|20hex|20hex|
| Detección de paquetes habilitada |20c3chex|200hex|0|
| Árbitro CCK habilitado |20c80hex|80000000hex|0|
| Antenas RX AB |26b54hex|fhex|3|
| CCA RX AB |26b50hex|3c000hex|c000hex|
| CMAC RX, interfaz PHY y DMA habilitados |10000hex|10000030hex|10000030hex|

Fuente fijada c03c1a14507dfaf8b856c2608f9ac216021fc222:
`rtw8922a.c:1849-1865,1981-2004,2162-2179`, `reg.h:6574-6595,
8884-8893,9017-9018,9040-9041,10016-10025`. BB usa la base0x20000.
Los ocho controles coinciden en las tres fases de ambas ejecuciones.
No son una captura directa del handshake PHY→MAC. PMAC_GNT/MAC_SEL no
reciben expectativas inventadas a partir de nombres de campos.

## Recuperación del estado inicial

El adaptador estaba en0bda:1a2b. El usuario ejecutó el helper autenticado,
que verificó disk16 como CD virtual Realtek y obtuvo DKIOCEJECT=0.
Inicialmente seguía enumerado como instalador. La herramienta mode-switch
hizo un envío allow-removal y no obtuvo CSW válido: recibió18 bytes con
forma de sense y luego error. No se considera una finalización SCSI correcta.
La enumeración posterior sí verificó37ad:0104, 802.11be WLAN Adapter.
No se atribuye el cambio exclusivamente a ese envío o a la expulsión previa.

La inspección inicial mostró status3/run_environment0 y BBMCU en reset.
El modo powered de radio-init se detuvo por perfil de alimentación no coincidente
antes de reset. El modo cold se detuvo porque reset_to_usb2_scc exige firmware
de investigación previamente cargado. Su mensaje externo dice que comenzó la
transición, pero la traza muestra rechazo en la primera comprobación0x1e0,
antes de las escrituras de parada DMA/reset; la ventana de alimentación se
restauró. No se reutilizaron las secuencias reservadas por esos intentos.

Se completó entonces esta secuencia, con salidas0:

1. `archer-firmware-boot-stage63 --boot-usb-no-dcpu`: carga inicial del firmware
   principal y BBMCU, con la omisión experimental DCPU ya documentada.
2. `archer-radio-init-stage42 --cold-scc-radio artifacts/radio-init-stage63c`:
   reset coordinado SCC, nueva carga, DLE/HFC/DMAC/CMAC, BB/radio/NCTL.
3. `archer-rfk-stage42 --initial-rfk artifacts/rfk-stage63`: DACK/RXDCK inicial limpios.
4. `archer-tx-calibrate-stage42 --channel1-world-calibrate artifacts/tx-rfk-stage63`:
   comandos completados, manteniendo los desconocidos de calidad DPK/TSSI.
5. Dos ejecuciones de `archer-context-rx-stage63` con
   `--run-owned-source-profile-usb-aggregation-all-management-async`, usando
   directorios nuevos `artifacts/context-rx-stage63` y `artifacts/context-rx-stage63b`.

Las versiones residentes se vinculan por el historial de carga controlado;
no se añadió lectura independiente de identidad. El ledger no se reinició.

## Recepción y verificación independiente

| Medida | Primera ejecución | Repetición sin recargar firmware |
|---|---:|---:|
| Transferencias USB guardadas |380|287|
| Paquetes normales |276|279|
| Beacons decodificados |228|220|
| Probe responses decodificados |29|48|
| Otros normales no clasificados por el parser acotado |19|11|
| FCS IEEE CRC32 coincidentes |276/276|279/279|
| Errores CRC/ICV de descriptor |0/0|0/0|
| Cabeceras convertidas |0|0|
| ACK de contexto |8|8|

Los totales abarcan todas las fases del ejecutor, no solo la ventana de cinco
segundos. El decodificador offline recorre todos los ficheros RX, verifica sus
límites y aplica la política FCS appended sustentada por APPEND_FCS=1.
Además se calcula zlib.crc32 sobre el payload sin sus últimos cuatro bytes y
se compara con esos cuatro bytes little-endian. Coinciden los555 paquetes.
No se infiere que los30 paquetes no clasificados sean inválidos: el parser
solo acepta un subconjunto de beacon/probe response. No se publican SSID/BSSID.

Ambos ciclos retiraron el contexto, conservaron TX pausado y acabaron con
reserva CLEAN, salida0 y stderr vacío. Las nueve lecturas asíncronas pendientes
se abortaron y drenaron en cada cierre. Los24 campos de la auditoría RMAC/PLE
volvieron a su valor inicial en cada ciclo.

En las ventanas de5019/5013 ms, CRC OK PHY aumentó CCK278/276 y OFDM6/9;
HT0/0. Cuota6 PLE alcanzó uso3/2 en las muestras. Los flags RMAC, PHYINFO y
CMAC fueron0 en todas las16 fases de cada ciclo. Dispatcher c0hex persistió
**también con recepción funcional**: ese flag por sí solo no demuestra bloqueo.

## Qué demuestra y qué queda abierto

La recepción de gestión normal, su entrega por USB, los límites del descriptor,
la cabecera802.11 y el FCS son reproducibles en este perfil sobre Mac ARM.
La recuperación completa y el reinicio del estado precedieron al éxito. No hay
un contraste que aísle qué cambio resolvió el estado sin RX anterior; no se
presenta una corrección causal de un registro concreto ni se concluye que
reconectar sea una solución general. Falta integrar una secuencia de arranque
coordinada y probar recuperación frente a errores/desconexiones.

50 suites C++ y55 tests Python pasan; compilación arm64/x86_64. La prueba de
auditoría cubre ausencia de escrituras y detención por fallos de lectura.
Intel solo compilado; DPK/TSSI siguen sin semántica completa de calidad.

El avance ponderado pasa a **45 %**: la ruta de datos obtiene5/15 por recepción
normal repetida; TX y el resto del camino de datos siguen pendientes. El siguiente
paso es convertir estas capturas de gestión en un escaneo pasivo estructurado,
con resultados agregados y ciclo de vida reproducible, antes de asociación/TX.

Se conserva el manifiesto de preparación original como historial de la espera.
Evidencia física, hashes y verificación: [phy-rx-stage63-observation.json](phy-rx-stage63-observation.json).
