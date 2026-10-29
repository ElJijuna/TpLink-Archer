# Etapa75 — primera entrega USB de trama y reportes TX reales

2026-10-29. Hardware Mac ARM, USB37ad:0104. Avance50 %: **TX no exitoso**.

## Resultado tangible

Se entregó por endpoint0x05 una solicitud de sondeo broadcast de96 bytes
(descriptor64+trama32). Hubo **una escritura USB**, status0. El firmware emitió
cuatro C2H MAC/MISC/TX_REPORT de80 bytes, etiqueta1, estado1
(`RTW89_TX_RETRY_LIMIT`) y contadores1/2/3/4. No son cuatro duplicados idénticos.
El límite de reintentos configurado a1 no produjo la conducta esperada;
una sola escritura USB no implica un solo intento por radio.

No hay TX_DONE, ACK externo ni captura por un segundo receptor que pruebe
emisión por aire. El destino broadcast tampoco habría permitido afirmar
ACK de un AP. No se realizó asociación ni se enviaron datos de usuario.

El scheduler se habilitó solo para MGQ0 y se volvió a pausar al fallar.
Una lectura física independiente posterior confirmó **0x10398=0**.
La reserva de contexto quedó **DIRTY**: la retirada de ROLE se comenzó,
pero el procesamiento del segundo reporte interrumpió la confirmación.
No se ha marcado CLEAN ni se han reutilizado las secuencias.

## Preparación y límites

`single-probe-universal` compila el flujo de contexto existente con
ARCHER_SINGLE_PROBE. Se habilita el reporte de gestión en CMAC G7 mediante
un argumento opcional del constructor de contexto; el camino RX normal
conserva el valorfalse. La admisión exige el perfil de firmware/radio conocido,
CV1/AID6933, canal1, TX pausado y coincidencia de las tablas world residentes.
Esta comprobación no equivale a medir potencia irradiada o validar calidad RF.

El primer intento se detuvo al no conseguir silencio en USB, **sin comandos
de contexto ni trama enviada**. Se corrigió el drenaje cerrando temporalmente
los filtros RX y restaurándolos. El segundo intento construyó el contexto,
envió la trama y obtuvo los reportes descritos. Se reservaron9 secuencias por
intento; los primeros índices no se reciclan aunque no se hubieran enviado.

Antes del envío se creó y sincronizó un marcador O_EXCL de una sola prueba.
La versión final rechaza ese marcador antes de crear un nuevo contexto;
el rechazo se probó sin tocar USB. **No eliminar el marcador para repetir**:
primero resolver recuperación y semánticas de reintentos.

## Cambios posteriores al ensayo

La revisión de core.c:910-940,1306-1308 identificó defaults de gestión omitidos
en el paquete ensayado: WD_PAGE, SW_MLD y DISDATAFB. Se corrigieron en el
generador nativo; **esa versión aún no se ha enviado al hardware**. No se
atribuye el fallo a esos campos sin prueba. El paquete realmente enviado
se conserva en `artifacts/single-probe-stage75b/single-probe.bin`.

También se desactiva el procesamiento de reportes TX al pausar el scheduler,
para que reportes tardíos no interrumpan la retirada del contexto. Se conserva
el rechazo conservador de reportes múltiples durante el envío. La recuperación
física del contexto DIRTY sigue pendiente; no se continuó transmitiendo.

La recompilación final sustituyó el ejecutable de prueba; no se conservó un
hash previo de ese binario. Los logs, el paquete enviado y los reportes sí
se retuvieron. No confundir los bytes de la versión final con los ensayados.

## Evidencia y validación

Fuentes fijadas del mismo commit de etapas72-74: mac.c:5795-5824,
fw.h:4146-4171, core.h:4676-4679, reg.h:6908-6919 y core.c citado arriba.
Parser: categoría1, clase9, función1, tamaño80, tag en w12[15:12], estado
en w12[9:8] y contador en w14[15:10]. Tag1 es distinto de la secuencia Wi-Fi1.

[single-probe-stage75-observation.json](single-probe-stage75-observation.json)
registra hashes y resultados. Logs detallados en `artifacts/single-probe-stage75*`;
reportes extraídos en `artifacts/probe-reports-stage75.jsonl`; comprobación
posterior en `artifacts/probe-stop-audit-stage75.jsonl`.

Build universal ARM/Intel correcto, Intel sin prueba física. `make test` pasa
(76 tests Python y suites C++ con el nuevo test de paquete/reporte). No se
repite RF para validar cambios puramente de software. Sigue pendiente
recuperar el contexto y determinar los reintentos antes de otro ensayo.
