# Etapa80 — contraste de respuesta externa y tasas básicas

2026-10-30. Avance52 %. Auditoría de capturas existentes; ningún nuevo TX.

Se añadió `ProbeAirAudit` y el ejecutable universal
`archer-probe-air-audit-universal`. Reproduce los RX contiguos del ensayo de
etapa79 y comprueba encuadre RX, política FCS y CRC32 por software antes de
contar probe responses cuya DA coincida con02:00:00:89:22:a1.
Conserva separados el canal recibido y el DS anunciado; el perfil se limita
a la captura de canal1 del experimento. No interpreta falta de respuesta
como prueba de que no hubo emisión por aire.

```
make probe-air-audit-universal
./build/archer-probe-air-audit-universal --channel1-fcs-appended artifacts/single-probe-stage79/capture
```

Resultado:241 observaciones de gestión aceptadas, bad_fcs0,
**0 probe responses dirigidos a la MAC de prueba**. Se observaron beacons
de16 BSSID que anuncian DS1. No se detectaron tasas básicas anunciadas fuera
del conjunto CCK que incluía la solicitud enviada (1/2/5,5/11Mbps).
Eso no respalda la hipótesis de que la falta de tasas básicas en la solicitud
explique el silencio; tampoco excluye otras diferencias de capacidades.

La auditoría anterior de etapa79 se hizo con un helper puntual. Esta versión
integra la comprobación FCS por software y pruebas reproducibles en el repo;
el resultado de respuestas dirigidas sigue siendo0. No es una nueva captura.

Evidencia local: `artifacts/probe-air-audit-stage80.json`. Los tests comprueban
DA de la estación frente a respuestas a otros clientes, CRC incorrecto,
metadatos fuera de límites, canal no admitido y tasas básicas OFDM omitidas
en un vector sintético. `make test` pasa:81 tests Python y suites C++ con
el nuevo test de auditoría. Log `artifacts/probe-air-tests-stage80.log`.

El usuario indicó **Especial** y confirmó que lo ve visible. No se pidió
contraseña ni se reutilizó el permiso de un solo uso de etapa79.

## Canal candidato y nueva captura detenida

Especial no aparece en los JSON completos de etapas78/79. El sondeo dirigido
CoreWLAN desde el Wi-Fi integrado devolvió un resultado en canal9/2,4GHz,
con SSID no legible. El SDK describe el parámetro como SSID de la sonda;
no basta para confirmar el nombre del resultado. **Canal9 es candidato**.
Evidencia: `artifacts/especial-corewlan-details-stage80.json`. Esta consulta
pudo emitir sondas del Wi-Fi integrado; no hubo TX del Archer ni asociación.

Una captura nueva en1 se detuvo porque continuaban241 paquetes tipo1/PPDU
status aun con filtros de tramas cerrados. Se añadió al journal de drenaje
el bit0 de0x11440, conforme a mac_be.c:2706-2725. Se restauran los cuatro
campos antes de crear contexto. El siguiente intento superó el drenaje,
pero MACID pause/sleep no recibió confirmación: contexto DIRTY y captura
completa no disponible. No se envió otra trama de red. La lectura posterior
confirmó scheduler TX0. Logs `artifacts/target-special-channel1-stage80{,b}.*`
y `artifacts/tx-paused-stage80.jsonl`.

Se preparó **1→9→1 sin prueba física**. Los umbrales Barker0x206c5 y
CCK0x2c94f son el índice8 de rtw8922a.c:1139-1147; frecuencia2452MHz,
SCO107. El nuevo modo `--owned-slot0-channel1-9-1` conserva RXDCK por destino
y retorno verificado; el decoder admite `--channel9-fcs-appended`.
No se amplió TX ni su tabla de potencia a9. Build universal y pruebas de
planes9 correctos; `make test` pasa. Log `artifacts/channel9-build-tests-stage80.log`.

Se solicitó reconexión eléctrica para recuperar el contexto y validar
pasivamente el candidato9. El reset en caliente sigue sin causa resuelta.

TX_DONE de etapa79 sigue siendo evidencia del firmware, sin respuesta
externa ni asociación. La identificación del AP objetivo es el siguiente
paso para discriminar formato, política del AP y funcionamiento RF.
