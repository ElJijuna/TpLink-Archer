# Etapa86 — contraste unicast: límite de reintentos, sin éxito confirmado

2026-10-31. Mac ARM. Avance52 %. No conexión Wi-Fi utilizable.

## Auditoría previa

La captura85 confirma que Especial anuncia tasas básicas1/2/5,5/11Mbps,
exactamente las cuatro CCK del probe. No faltan tasas básicas exigidas por
este AP. Anuncia además las ocho tasas OFDM como soportadas, no básicas.
Se amplió el auditor offline para conservar estos conjuntos en unidades
 de0,5Mbps. Esto descarta esta variante concreta de la hipótesis de tasas;
no certifica que el conjunto completo de IEs sea suficiente para todo AP.

Se contrastaron los campos del descriptor con la fuente Linux fijada:
`core.c` builders v2 y defaults de gestión, `txrx.h` máscaras BE v2,
`usb.c` STF y selección de endpoint. No se identificó una discrepancia
concreta en longitud, QSEL/DMA, WD_PAGE, SW_MLD, tasa, cabecera o reporte.
No se añaden BMC/NO_ACK a body6 por conjetura: el builder v2 de fuente
solo codifica UPD_WLAN_HDR allí; is_bmc cambia RTS_EN de info4.

## Experimento nuevo, una sola trama

El broadcast85 no exige ACK. Para contrastarlo se ejecutó un único probe
unicast al BSSID ya recibido50926a110666, conservando SSID Especial,
canal9 y tasaCCK1. DA/BSSID apuntan al AP; secuencia3,tag3,104 bytes.
Info4 pasa de0x80000000 a0x88000000, como el builder de Linux para !is_bmc.
Esto cambia también la elegibilidad RTS; el resultado no aísla ACK frente
a todas las posibilidades de fallo RTS/CTS/datos.

Modo `--unicast` del coordinador: exige etapa85 completada y su log íntegro,
ledger idéntico posterior (0..40 reservadas), evidencia84 íntegra y contexto
CLEAN. Usa un tercer marcador permanente distinto; no elimina ni reutiliza
los intentos anteriores. El binario hereda su FD y verifica canal/potencia.

Resultado físico:

- Una entrega USB endpoint05,104 bytes,status0.
- Un reporte **tag3,state1,count1**.
- `core.h:4676–4677`:0=TX_DONE,1=TX_RETRY_LIMIT. `mac.c:5795–5860`
  reconoce el formato8922A y trata el fallo como terminal al alcanzar el límite.
- No hubo terminal de éxito ni respuesta externa a nuestra MAC.
- Captura recibida:72 tramas de gestión aceptadas,FCS0; del objetivo29 beacons
  y2 probe responses, ninguno dirigido a nuestra MAC.

El contraste es informativo: el éxito del broadcast85 no equivalía a
comunicación bidireccional verificada. Este resultado tampoco demuestra por
sí solo falta de emisión RF ni permite distinguir una pérdida de respuesta
 en aire de errores de recepción/control o de RTS/CTS.

## Estado final

Los comandos de retirada6/7/8 recibieron confirmación hasta paso9.
Última lectura de0x10398=0: scheduler de red pausado. Callbacks drenados.
El fallo del probe se propaga después de la retirada, antes del commit CLEAN;
por ello **la reserva queda DIRTY**, aunque se conservan los ACK de limpieza.
Quedan206 secuencias. Canal9. No borrar la reserva ni repetir el envío.

## Validación y evidencia

`make test`:87 tests Python y suites C++. Nuevos tests de direcciones,
secuencia,tag3,flagsRTS y rechazo de reportes de otro tag. Compilación
universal de las variantes de captura, probe1, probe9 y unicast9 correcta;
Intel sin prueba física.

[special-unicast-stage86-observation.json](special-unicast-stage86-observation.json)
conserva resultados, limpieza, lectura TX, hashes de fuente primaria, binario,
trama y capturas. Artefactos en `artifacts/special-unicast-probe-stage86/`.

Siguiente: recuperar el contexto de forma verificable antes de más H2C y
contrastar la emisión con otro receptor o instrumentar contadores TX/RX de
respuesta documentados. Mantener separadas las hipótesis RF, RTS/CTS y
recepción de ACK; no asumir que el firmware TX_DONE broadcast prueba ACK.
