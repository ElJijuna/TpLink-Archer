# Canal de comandos — cuarta etapa

## Implementado

`include/archer/firmware_channel.hpp` proporciona tres operaciones sin I/O:

- Construir H2C con cabecera de 8 bytes, categoría/clase/función, secuencia y flags ACK explícitos. Rechaza campos o tamaños que no caben en sus bits.
- Envolver H2C en un descriptor BE v2 de 24 bytes, tipo 13. Añade cuatro bytes cuando el tamaño USB sería múltiplo de 512; conserva la longitud H2C y ajusta la del descriptor.
- Decodificar transferencias RX BE v2 con descriptores de 24/40 bytes, desplazamientos de metadatos y alineación de agregados a 16 bytes. Para paquetes tipo 10, valida la cabecera C2H y su longitud antes de exponer categoría/clase/función.

Las fuentes son snapshots locales de `fw.c`, `fw.h`, `core.c`, `core.h`, `txrx.h` y `usb.c`. El código preserva el alcance: no administra asociación, secuencias pendientes, retransmisiones ni semántica de comandos. No hay envío H2C al dispositivo en esta etapa.

## Corroboración Windows con REA

En el SYS oficial de SHA-256 `68f735cc46b25ce983997d7f8693b24ea75af3abaf42a2f4d69fb63ad5590332`:

- `ev_5831ff73aeee4a358d9e60614835611034a2643413950a90d6e2c4bff11e3330` localiza `h2c_pkt_set_hdr` en `0x1404ca880`.
- `ev_26e0b3076ce30d9e7eeaa5237ab62325ae03b89a7bdf2d98dbf995c79f518193` enlaza esa cadena con `0x14035ef3f`.
- `ev_59cbd111a0e0ae78c2fc0da55c45f4cdd7a7db27b4eb769aaa6f4978976b0163`, función `0x14035eed0`, muestra la inserción de ocho bytes y campos compatibles con la cabecera estudiada. El pseudocódigo de flags incluye artefactos del descompilador; para codificación exacta se usa la referencia Linux.
- `ev_88ead076a548c9bea0baa74df49c18c213c8dc5fd77690ef2d18ee07c0e7ded3` corresponde a la ruta de impresión de versión. Esta llama una función interna; no demuestra una consulta de versión por USB.

El bundle `rea-channel-evidence.json` conserva seis registros completos. Una consulta adicional de análisis no terminó y su cierre REA se solicitó sin recibir confirmación; comprobar `binary_session` antes de reutilizar esa sesión. No se usa esa consulta incompleta para concluir cómo obtener la versión del firmware en ejecución.

## Endpoints y prueba real

Linux asigna `RTW89_DMA_H2C` al índice 2 del array de salidas bulk. Con el orden de descriptores observado eso corresponde a `0x07`. Es un candidato basado en fuentes y descriptores; no ha sido validado mediante envío a este PID.

`archer-receive-universal` abrió normalmente la interfaz 0 del USB `37ad:0104`, sin seize ni cambios de configuración. Enumeró referencias de pipes en tiempo de ejecución, y realizó una única lectura por entrada, con 500 ms de timeout sin datos y 1000 ms de timeout total solicitado por lectura:

| Endpoint | Referencia pipe | Resultado | Bytes |
| --- | --- | --- | --- |
| `0x84` | 1 | timeout `-536854447` | 0 |
| `0x88` | 5 | timeout `-536854447` | 0 |

La interfaz se cerró correctamente. No se envió bulk OUT, comandos de firmware ni escrituras de registro. Las lecturas consumen datos si existen; no son una captura pasiva. El resultado solo establece ausencia de datos en ese intento, no ausencia de firmware o de eventos futuros. El registro está en `receive-observation.jsonl`.

## Uso y códigos de salida

```sh
make test receive-universal decode
./build/archer-decode tests/fixtures/synthetic-c2h-be.bin
./build/archer-receive-universal /tmp/archer-receive-new
```

El directorio de recepción debe ser nuevo. Se guardan `receive.jsonl` y los bytes recibidos por endpoint, si los hubiera. Las capturas pueden contener datos del dispositivo; conservarlas en `artifacts/`, excluido de Git. Un fallo de decodificación conserva los bytes sin afirmar que sean C2H.

Salida del receptor: 0 = recibió bytes (no garantiza C2H válido), 1 = dispositivo ausente, 2 = error o selección ambigua, 3 = ninguna entrada produjo datos. `--help` no accede a USB. El acceso USB puede requerir ejecutar fuera de un sandbox; no necesita `sudo` en el host probado.

El fixture `synthetic-c2h-be.bin` es sintético. No representa una respuesta observada ni una consulta de versión validada. Las pruebas cubren encabezados, longitudes, padding 512, agregación, metadatos, truncamiento y rechazo de entradas ajenas. Pasaron AddressSanitizer y UndefinedBehaviorSanitizer. El receptor compila para Intel y ARM; la ejecución se probó únicamente en ARM.

## Pendiente para comunicación activa

Identificar un comando de consulta válido para la versión ejecutada; corroborar su payload, ACK y condiciones de disponibilidad H2C; configurar el canal si fuera necesario; y verificar una respuesta C2H real. No se inventó un opcode de consulta de versión a partir de nombres de logs ni se trató el estado de firmware como garantía de disponibilidad del canal USB.

## Stage 10: physical USB bulk H2C/C2H receipt acknowledgment

ARM hardware execution sent one 44-byte USB transfer through endpoint 0x07
(IOKit pipe reference 4), using the BE 24-byte TX prefix, 8-byte H2C header
and 12-byte LOG_CFG payload. Command category=MAC 1, class=FW_INFO 0,
function=LOG_CFG 0, sequence=0. Payload level=4, path=2, component and extended
component masks=0: the Linux log-disable semantics. Receipt and completion ACK
flags were requested. This changes firmware log configuration; the previous
configuration is unknown and was not restored. No Wi-Fi scan or association
was initiated and no bulk command retry was performed.

The write returned status 0. Endpoint 0x84 returned 36 bytes: a 24-byte RX
descriptor, packet type 10, and a 12-byte C2H packet. Its header identifies
MAC INFO REC_ACK (category 1, class 0, function 0). Its acknowledged H2C fields
match category 1/class 0/function 0/sequence 0. The upper header byte 0x4b is
not the acknowledged H2C sequence; that sequence is extracted from REC_ACK
word 2 bits 23:16. No DONE_ACK was observed in this bounded single-read test.
Endpoint 0x88 timed out. WLAN IO returned to 0x001004fd.

This validates one actual bulk command and its receipt acknowledgment on ARM;
it does not establish command completion, stable repeated sequencing, radio
initialization, scan or networking. Intel is compilation-tested only.

`archer-receive-universal --diagnostic-log-disable NEW_DIRECTORY` implements
this explicit diagnostic. It requires ready DLE managers, enabled HCI TX DMA,
and the expected nine-endpoint USB2 inventory with bulk OUT 0x07/512 bytes.
It preserves the outgoing bytes and raw RX before decoding. No arbitrary
command injection is exposed. Exit 4 means no matching receipt ACK; exit 0 in
this mode requires matching ACK, whereas ordinary receive mode still only
requires bytes. The test capture predates this stricter exit condition; the
new decoder is verified against that exact physical fixture offline.

Evidence: `bulk-command-observation.json`; real regression fixture:
`tests/fixtures/log-disable-receive-ack-usb.bin`. Source definitions:
fw.c `rtw89_fw_h2c_fw_log`, fw.h SET_LOG_CFG fields and REC_ACK word-2 masks,
mac.h INFO function enum. Hopper remains unavailable due to license expiry;
no new native REA analysis is claimed.

Next: implement a bounded event loop with independent receive/completion ACK
tracking and sequence ownership, then establish firmware identity and radio
bring-up prerequisites before issuing scan commands.
