# Etapa68 — barrido pasivo1→6→11→1 probado y reproducido

Fecha: 2026-10-27. El nuevo ejecutor realiza transiciones de recepción,
RXDCK por destino, capturas separadas y retorno comprobado al canal1.
Se probó primero1→6→1 y después dos ciclos completos1→6→11→1 sobre Mac ARM.

## Resultado físico

Dwell nominal de tres segundos por destino, con verificación FCS antes de
aceptar beacon/probe response:

| Ciclo | Canal6: entradas/observaciones | Canal11: entradas/observaciones | Canal1 restaurado: entradas/observaciones |
|---|---:|---:|---:|
| stage68, primer ensayo |15/133|—|11/97|
| stage68b, barrido completo |15/143|18/186|12/120|
| stage68c, versión final repetida |13/127|19/144|13/100|

Todos los dwells tienen bad_fcs0. La reproducción offline coincide exactamente
con cada uno de los ocho JSON en vivo. Las entradas usan BSSID+SSID+canal DS;
no se suman sus conteos como puntos de acceso distintos. La variación entre
ciclos no prueba aparición/desaparición de redes: son ventanas cortas.

Cada destino emitió PRE_NOTIFY y RXDCK serializados; el informe RXDCK confirmó
band0, ancho20MHz, canal esperado y timeout0 en A/B. El primer ciclo consumió
cuatro secuencias y cada barrido completo seis:232→216 disponibles, sin wrap.
Las tres ejecuciones salieron0, stderr vacío, contexto/reserva CLEAN y TX de
red pausado. Se verificó el retorno digital RF A/B a1 y BB central2412 MHz.
La versión final también exige CV1, AID6933, DMAC/HCI y estado NO_LINK inicial.

Los readbacks de SYN A analógico y ambos registros digitales coincidieron con
6/11/1. RF B analógico permaneció0, como en las pruebas previas; se conserva
esa observación sin exigir que sea el sintetizador seleccionado en SCC.

## Flujo implementado

`src/macos_passive_retune.cpp` mantiene SequenceLedger y la reserva durable
existente durante todo el ciclo. La opción declara el historial controlado de
slot0/canal1 y slots1/2 sin uso; no descubre tablas MCC por introspección.
Se exige el perfil recuperado/calibrado conocido de la investigación y no se
pretende aceptar un adaptador en cualquier estado.

Antes del retune cierra el canal USB asíncrono y drena sus callbacks/buffers.
Deshabilita filtros de gestión/control/datos, entra al reset bracket y drena
transferencias viejas mientras la detección BB está detenida. Después aplica
el plan BB de etapa67, ajusta RF y comprueba readbacks, sale del bracket y
aplica la selección post-RF con historial MCC. La entidad única de destino
permite reemplazar slot0 en este recorrido; no se usa un atajo genérico que
ignore canales activos. No hay RX etiquetado como dwell durante la transición.

Antes de RFK comprueba que RF no permanezca en TX. Antes de cada comando
vacía eventos previos hasta observar50 ms sin transferencias, con límite de3 s.
El terminal RFK carece de identidad de comando/secuencia; se conserva esa
limitación y no se afirma que el drain resuelva cualquier reporte tardío.
RXDCK requiere además el detalle de canal/timeout de las dos rutas, no solo OK.
Los H2C y las transferencias de control se conservan por transición.

El historial de canal solo se confirma después del RXDCK limpio. Entonces se
habilita la ventana pasiva y se captura el dwell. Al cerrarlo, se abortan/drenan
las lecturas antes del próximo cambio. Al final, las ventanas existentes
restauran filtros, response engine, puerto, PHY reports y agregación USB.
Esto es retorno a un canal comprobado, no rollback bit a bit de toda la RF:
RXDCK y los estados de calibración son operaciones con estado.

Una transición o un RFK incierto detiene el proceso, conserva DIRTY y TX
pausado. No intenta retunear automáticamente desde un estado parcial ni
reutiliza las secuencias reservadas. Esos fallos físicos no fueron inyectados
en esta etapa. La captura normal no crea ROLE/CAM sintéticos nuevos: este
perfil recibe gestión con las ventanas pasivas y el estado controlado existente.

## Alcance RF y procedencia

No se aplican las tablas TX de potencia de canal1 a6/11 ni se envían IQK,
TXGAPK, TSSI o DPK de canal6/11. Solo se ejecuta RXDCK para el ensayo de
recepción. Las correcciones de calibración/tabla existentes no se presentan
como calidad RF validada para los nuevos canales. No se habilita scheduler TX,
no se construyen probe requests y no se afirma ausencia de toda señal RF
interna de calibración. TX, asociación y potencia de producción siguen fuera
de este barrido diagnóstico.

PassiveScan ahora recibe un canal explícito1/6/11 por captura y conserva el
canal DS anunciado por separado. No lo deduce del descriptor. El parser de
RXDCK añade clean_2g20(expected_channel), manteniendo la comprobación antigua
de canal1 como wrapper. ContextReservation se comparte en una cabecera sin
cambiar el protocolo DIRTY/CLEAN del ejecutor anterior.

## Uso

```
make test passive-retune-universal context-rx-universal passive-scan-decode-universal
./build/archer-passive-retune-universal --owned-slot0-channel1-6-11-1 artifacts/NEW_SWEEP
./build/archer-passive-scan-decode-universal --channel6-fcs-appended artifacts/NEW_SWEEP/channel-6
```

La opción corta --owned-slot0-channel1-6-1 conserva el ensayo inicial. Cada
intento exige un directorio nuevo. Los resultados están en channel-N/networks.json;
el ejecutor de escaneo anterior y el coordinador Python mantienen su modo
anterior de canal1. El nuevo barrido es un ejecutor nativo separado que requiere
el perfil ya preparado; todavía no es una operación de la interfaz macOS Wi-Fi.

53 suites C++ y64 tests Python pasan; se amplían pruebas de procedencia del
canal de recepción frente a DS y de RXDCK6/11, timeout y canal divergente.
Build universal correcto; hardware solo ARM. No se ha probado retune en Intel,
5/6GHz, otros anchos ni todos los canales2.4GHz.

Avance ponderado **50 %**: escaneo/asociación pasa de3/15 a5/15 por barrido
pasivo1/6/11 con retorno verificado. El próximo paso es integrar este barrido
en el coordinador y consolidar resultados por BSSID/procedencia antes de TX
y asociación.

Evidencia, hashes y resultados: [passive-retune-stage68-observation.json](passive-retune-stage68-observation.json).
