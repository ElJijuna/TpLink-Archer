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
