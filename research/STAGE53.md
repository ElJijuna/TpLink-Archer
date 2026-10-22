# Etapa53 — tablas por MACID del RTL8922A

Fecha: 2026-10-22. Fuente Linux fijada a
`c03c1a14507dfaf8b856c2608f9ac216021fc222`.

## Inicialización directa y H2C

`mac.c:4403-4435` omite la inicialización directa DMAC/CMAC por MACID cuando
el chip no es AX. Por tanto, RTL8922A BE no debe recibir esas escrituras
indirectas AX como parte de la reconstrucción de interfaz.
`rtw8922a.c:3180,3185` selecciona los comandos CMAC G7 y DMAC V2.
Son distintos de la inicialización global DMAC/CMAC ya ejecutada.

`interface_tables.hpp` implementa offline:

| Comando | Payload | Cat/clase/función | ACK solicitado |
|---|---|---|---|
| CMAC G7 predeterminado | 132 bytes | 1/5/11hex | finalización |
| DMAC V2 predeterminado | 132 bytes | 1/5/0chex | ninguno |
| MACID_PAUSE_SLEEP | 256 bytes | 1/9/28hex | recepción |

CMAC y DMAC tienen c0,16 palabras de valores y16 máscaras. MACID es7 bits,
OP=1. Las máscaras preservan los campos reservados: no se sustituyen por
ffffffff indiscriminadamente. CMAC reproduce los defaults de tasa/reintento,
subcanales, padding, respuesta y CSI; tx_report_enabled es un parámetro
explícito del encoder. No se habilita TX mediante esta función offline.
DMAC mantiene valores0 y máscaras de la fuente, con m13..15 en0.

Fuentes: `fw.c:2463-2519,3663-3746`, `fw.h:1404-1544,4744-4752`,
`cam.h:188-303`. El allocator fuente pone a cero todo el payload.

## Pausa/sueño individual

La imagen0.35.41 selecciona MACID_PAUSE_SLEEP por la regla
`fw.c:909` (RTL8922A>=0.34.11). `fw.c:5180-5227` y `fw.h:320-327`
definen cuatro bloques de64 bytes. Solo se usan los campos de n[0] y se
enmascara un único MACID: grupo=MACID/32 y bit=MACID%32. La variante de
despausa limpia pausa/sueño de ese índice mediante sus máscaras, sin
modificar otros MACID. No reanuda el scheduler global.

El rango común de una futura transacción de interfaz queda0..127; los
encoders ROLE/JOIN admiten más bits por formato, pero eso no amplía los
recursos del chip ni los campos de estas tablas.

## Validación y siguiente ejecución

46 suites C++ y46 tests Python. Comparación de todas las35 palabras de cada
frame CMAC/DMAC, incluidas cabeceras y máscaras. Se verifican los64 words
del payload de pausa/despausa, índices0/127 y rechazo128. Pasaron ASan/UBSan
y compilación universal ARM/Intel del test.
Registro y hashes: [interface-tables-stage53.json](interface-tables-stage53.json).

No hubo acceso USB en esta etapa. No se afirma aceptación por firmware,
contenido real de tablas ni mejora RX. Próximo paso: coordinador de ciclo
de vida que reserve MACID/CAM, serialice pausa→ROLE→JOIN→CAM→CMAC→DMAC,
distinga ACK de recepción/finalización/envío sin ACK y retire ROLE/CAM ante
una salida controlada. Un timeout deja el estado incierto y no autoriza
reintentar creación ni declarar que el contexto se eliminó.
El perfil de puerto existente aún es diagnóstico parcial de NO_LINK,
no la implementación completa de `mac_port_update`.
Avance ponderado40%; último resultado físico RX USB0 en etapa51b.
