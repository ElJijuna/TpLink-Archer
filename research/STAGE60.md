# Etapa60 — contrato y telemetría de lecturas bulk

Fecha: 2026-10-22. Se instrumenta cada ReadPipeTO con endpoint real,
pipe, estado, tamaño solicitado y tamaño reportado. No se inspecciona ni
se cuenta el buffer cuando el estado es de error.

## Fuente y límite del contrato

El SDK local de Xcode, IOKit/usb/IOUSBLib.h:2672-2700, documenta que
ReadPipeTO no actualiza size al fallar y su buffer no contiene datos válidos.
También indica que un timeout aborta la petición y puede requerir resincronizar
el data toggle. Esto no demuestra que aquí exista una desincronización;
no se añadió una limpieza automática de stalls.

La fuente Linux fijada en c03c1a14507dfaf8b856c2608f9ac216021fc222,
rtw89-usb.c:506-542 y rtw89-usb.h:14-15, mantiene ocho bloques de recepción
asíncrona de20480 bytes sobre in_pipe[0]. El prototipo usa lecturas síncronas
alternadas sobre dos pipes con timeout de10 ms. Es una diferencia de mecanismo
que aún debe contrastarse; no identifica por sí sola la causa del bloqueo RX.

## Prueba física

Binario universal conservado antes de ejecutar el perfil de agregación USB
con recepción de todas las tramas de gestión, sobre Mac ARM:

| Endpoint | Lecturas | Éxitos | Bytes válidos | Timeouts |
|---|---:|---:|---:|---:|
| 0x84 | 400 | 8 | 288 | 392 |
| 0x88 | 400 | 0 | 0 | 400 |

Los792 errores fueron timeouts y conservaron size=20480. Ese valor es el
argumento original, no un recuento de bytes recibidos. Los ocho éxitos fueron
ACK de control; cero paquetes normales y cero beacons. No hay evidencia que
permita recuperar supuestos paquetes parciales desde los buffers fallidos.

En5014 ms, los contadores PHY CRC OK aumentaron CCK450, OFDM109 y HT6.
PLE grupo0 permaneció vacío y cuota6 usada0 en las21 muestras. Los flags
RMAC59hex, PHYINFO0ehex, CMAC40hex y dispatcher c0hex ya estaban en la
primera muestra; no aparecieron bits nuevos, lo que no excluye eventos
repetidos sobre bits activos. TX de red continuó pausado, el retiro de contexto
fue reconocido y la reserva terminó CLEAN. Stderr vacío y salida0.

## Validación y siguiente paso

49 suites C++ y55 tests Python pasan, incluida la exclusión de tamaños de
error, separación por endpoint y rechazo de observaciones inconsistentes.
Compilación arm64/x86_64 correcta; prueba física únicamente ARM.

El siguiente contraste es una cola de lecturas asíncronas persistentes,
con cancelación y vida de buffers/callbacks controladas. Esta etapa no la
implementa ni demuestra que vaya a resolver la recepción normal. El avance
funcional ponderado permanece en40 %.

Hashes del binario, fuentes, SDK, trazas y resultados:
[bulk-receive-stage60-observation.json](bulk-receive-stage60-observation.json).
