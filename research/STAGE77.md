# Etapa77 — RFK sin respuesta; firmware vivo por buzón

2026-10-30. Avance50 %. Ninguna nueva trama de red TX; marcador de prueba
de etapa75 intacto. El bloqueo RFK **no está resuelto**.

## Contrastes físicos

1. La enumeración mantiene37ad:0104. Los controles indican main status3 y
   BBMCU con boot-ready y resets liberados. No prueban actividad por sí solos.
2. Se cambió el receptor inicial RFK a MacBulkChannel persistente/asíncrono,
   con ocho solicitudes en84 y una en88 antes de enviar H2C. Tras una nueva
   época verificada, inspect/radio terminaron0 pero PRE_NOTIFY no tuvo terminal.
   El coordinador detuvo el flujo sin DACK, TX RFK ni scan.
3. Se encontró un plazo distinto del fuente: phy.c:3999-4011 multiplica por4
   la espera RFK para USB. `src/macos_rfk.cpp` se corrigió a ms*4; PRE_NOTIFY
   pasa20→80ms, DACK/RXDCK2000→8000ms. Tras otra época nueva verificada,
   PRE_NOTIFY volvió a quedarse sin terminal. La discrepancia se corrigió,
   pero **no se atribuye a ella la causa del fallo actual**.
4. Una consulta por buzón de registros devolvió capacidades PHY válidas:
   RX2/TX2. Acredita que el firmware principal procesa esa vía; no demuestra
   liveness de BBMCU ni que consuma/procese correctamente RFK por USB.

No se reenviaron los comandos pendientes dentro de sus épocas: cada contraste
de envío tuvo una frontera completa de reset/recarga sin H2C. Los ledgers
anteriores están archivados por el coordinador de épocas. No se borró el
marcador que bloquea otra solicitud de sondeo de red.

## Auditoría posterior sin nuevos H2C USB

| Registro | Valor |
|---|---:|
|0x1e0|0x0c000022|
|0x1f0|0x00001100|
|0x1f4|0x00001101|
|0x1f8|0|
|0x7880|7|
|0x8400|0xffff4e80|
|0x8430|0x0fffffff|
|0x8434|0x003c007d|
|0xb000|0x00080436|
|0xb010|0x80000000|
|0xb700|0x0000011b|
|0x10398|0|

HCI TX/RX/TRXBUF están habilitados; el bit de cuota PLE H2C vacía está1;
TX de red sigue pausado. Son snapshots, no prueban entrega RFK ni permiten
inferir por sí solos qué procesador o enlace falla. El buzón modifica su
secuencia/control propios: no es una lectura pasiva de registros únicamente.

## Evidencia

- `artifacts/rfk-preflight-stage77.*`: controles main/BBMCU.
- `artifacts/rfk-epoch-stage77/epoch.json` y
  `artifacts/async-rfk-workflow-stage77/workflow.json`: contraste asíncrono.
- `artifacts/rfk-usb-wait-epoch-stage77/epoch.json` y
  `artifacts/usb-wait-rfk-workflow-stage77/workflow.json`: plazo USB corregido.
- `artifacts/mailbox-live-stage77.*`: respuesta de capacidades PHY.
- `artifacts/h2c-audit-stage77.*`: estado posterior de controles/colas.

Build universal correcto; `make test` pasa:78 tests Python y suites C++.
Log `artifacts/rfk-async-tests-stage77.log`. La validación física es ARM;
no se afirma que las pruebas unitarias reproduzcan este timeout de hardware.

Se solicitó desconexión/reconexión USB para contrastar el reset eléctrico
con los resets en caliente. Siguiente: verificar enumeración, recuperar modo
Wi-Fi si aparece el instalador y probar desde carga fría conocida. No declarar
listo el escaneo/RFK hasta completar nuevamente sus confirmaciones.
