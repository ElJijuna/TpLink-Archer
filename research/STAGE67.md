# Etapa67 — planes de canal1/6/11 y búsqueda MCC preparados

Fecha: 2026-10-27. Se preparan subplanes puros para ampliar el escaneo pasivo.
**No se ha retuneado el adaptador ni ejecutado un barrido multicanal.**
El firmware y el ledger no se tocaron; permanecen232 secuencias disponibles.

## Delta de fuente comprobado

`include/archer/passive_channels.hpp` limita candidatos a PHY0/MAC0 SCC,
2.4GHz,20MHz, canales1/6/11. Construye el plan BB a partir de los mismos
datos de placa/ganancia validados de canal1 y sustituye cinco valores:

| Campo | Canal1 | Canal6 | Canal11 |
|---|---|---|---|
| Barker SCO,26758hex |1fe4fhex|20399hex|208e3hex|
| CCK SCO,2675chex |2bdachex|2c4f2hex|2cc38hex|
| Frecuencia central,26b4chex |2412|2437|2462|
| Recíproco redondeado,26b50hex |109|108|106|
| Índice de canal,20734hex bits23:16 |1|6|11|

Fuente fijada c03c1a14507dfaf8b856c2608f9ac216021fc222:
`rtw8922a.c:1138-1167,1466-1526,2179-2197` y `phy.c:8568-8592`.
Una comprobación offline extrae las dos tablas SCO directamente de esa fuente
y verifica los tres planes producidos con los datos reales enlazados de placa.
Cada uno conserva133 operaciones BB; canal1 es idéntico al plan anterior.
El resto de máscaras/direcciones/valores permanece igual.

La reutilización está acotada: los tres canales comparten subbanda de ganancia
2G (`phy.h:861+`), offsets CCK/OFDM, ancho20MHz, coeficientes CCK no14 y la rama
de deshabilitación de spur (la función spur_freq de este chip devuelve0).
No se extiende a canal14, otros anchos ni5/6GHz. Las comprobaciones originales
de entrada de placa/efuse siguen activas. Flags de prerrequisitos en el
exportador representan entrada para planificación, no evidencia de hardware.

El plan RF preserva todos los bits fuera de31fffhex de cada snapshot y coloca
el canal de destino, manteniendo100 us tras cada escritura. Está basado en
`rtw8922a_rfk.c:39-88` y `rtw8922a.c:2680-2721`, con CV1 requerido.
Las cuatro lecturas RF necesarias deben tomarse dentro del futuro reset bracket.
No se ejecuta RF sobre snapshots ficticios ni se omite el readback físico.

## Historial MCC

La fuente siempre llama chlk_reload incluso en SCC (`rtw8922a_rfk.c:208-324`).
El atajo de slot0 solo vale para un estado recién inicializado conocido.
`passive_mcc_lookup` recibe tres descriptores previos y los canales de las
entidades activas; nunca los deduce de registros. El algoritmo fuente
`phy.c:8337-8409` elige una coincidencia exacta aunque antes haya un slot libre;
en su defecto, el primer slot no usado por entidades activas; si todos están
ocupados, slot0. Un canal0 representa descriptor no inicializado.

Ejemplos comprobados:

- Historial[1,6,11], destino6, activa[6]: slot1 por coincidencia exacta.
- Historial[1,0,0], destino6, activas[1,6]: slot1, conserva el canal activo1.
- Historial[1,0,0], destino6, activa[6]: slot0 puede reemplazarse.

El post-RF puro conserva los índices elegidos y genera el contexto destino
para PRE_NOTIFY. No hace commit del historial ni certifica que sus coeficientes
estén calibrados. El coordinador deberá aplicar la selección y confirmar RFK
antes de dar un canal por listo; no se reutiliza calidad de canal1 como prueba
para6/11 ni se inventa una interpretación para DPK/TSSI.

## Herramienta y validación

```
make test passive-channel-plan-universal
./build/archer-passive-channel-plan-universal --inspect-plans
```

El exportador no enlaza IOKit ni usa USB. Emite executable=false y los bloqueos
que aún faltan integrar: historial MCC vivo, coordinación RFK/restauración,
contexto de potencia y procedencia del canal de captura. La opción no habilita
las tablas de potencia WW de canal1 para otros canales ni autoriza TX.

53 suites C++ y64 tests Python pasan. El nuevo test comprueba identidad del
plan1, delta de exactamente cinco valores en6/11, snapshots RF completos,
preservación de bits, límites de canal/cut/slots y búsqueda MCC con historial.
El exportador compila arm64/x86_64 y genera planes con los datos enlazados de
la placa; no es validación física del retune ni de Intel.

El siguiente paso es integrar estos subplanes en la transición de recepción,
con captura detenida durante retune, RFK/control de estado, etiquetado de cada
dwell y retorno verificado al canal1. El escaneo operativo sigue fijo en1 y
el avance ponderado permanece en48 %.

Hashes y contraste de fuente: [channel-plans-stage67-observation.json](channel-plans-stage67-observation.json).
