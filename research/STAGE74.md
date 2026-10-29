# Etapa74 — plan USB TX sin envío

2026-10-29. Avance50 %. Sin acceso USB ni transmisión física.

`usb_packet_plan` en `tools/be_tx_descriptor.py` une descriptor BE v2 y trama
sin relleno, exige longitud pkt_size coincidente y STF_MODE1, y resuelve
endpoint desde la configuración USB2 retenida. Rechaza canales sin entrada
explícita en la tabla del chip; no interpreta las entradas C omitidas como
un mapeo válido por defecto.

| DMA | Índice bulkout | Endpoint OUT USB2 |
|---:|---:|---:|
|0|3|0x09|
|2|5|0x0b|
|4|4|0x0a|
|6|6|0x0c|
|8,9|0|0x05|
|10,11|1|0x06|

Canal12/H2C queda excluido del camino de red. La fuente rtw8922au.c:18-28
define índices, no direcciones USB. usb.c:1025 enumera OUT en orden de los
descriptores. La configuración guardada del dispositivo contiene OUT
05/06/07/09/0a/0b/0c e IN84/88. El parser exige ese perfil, interfaz0,
alternate0 y endpoints bulk512; no generaliza a USB3 ni a otros adaptadores.

usb.c:308-345 presenta un skb por transferencia;393-434 añade el descriptor
sin agregación ni checksum adicionales en ese camino. usb.c:280 establece
URB_ZERO_PACKET. El plan marca zero_packet_required cuando descriptor+trama
es múltiplo512, sin sustituirlo por padding. **Todavía no hay implementación
macOS que envíe el payload/ZLP**. Esto no debe confundirse con el tratamiento
especial de paquetes firmware de351-390.

El callback usb.c:195-240 separa las tramas con reporte de las que terminan
por estado USB. Para las primeras, el éxito USB encola la espera de reporte;
no aporta por sí solo la confirmación de radio. El parser y la correlación
del reporte siguen pendientes, junto con los límites de potencia/calidad RF.

Uso offline:

```
python3 tools/be_tx_descriptor.py BODY.json --wdinfo-json INFO.json --frame-bin FRAME.bin --usb-config tests/fixtures/37ad-0104-usb2-config.bin
```

El plan incluye endpoint, bytes de transferencia y necesidad de ZLP;
hardware_validated=false/transmission_ready=false. El ejemplo local
`artifacts/tx-usb-plan-stage74.json` usa24 bytes cero, **no una trama Wi-Fi
válida**. No se envió. Fuentes/hashes en
[tx-usb-stage74-sources.json](tx-usb-stage74-sources.json).

Pruebas: todos los mapeos explícitos, borde512 exacto y513, rechazo de tamaño
discordante, STF incorrecto, configuración truncada, endpoint alterado y DMA
sin mapeo. `make test` pasa:76 tests Python más suites C++ existentes.
Log `artifacts/tx-usb-tests-stage74.log`. No se modificó el estado del hardware.

Siguiente: parser de reportes TX y correlación, antes de conectar este formato
offline con una prueba de envío controlada y contexto/potencia validados.
