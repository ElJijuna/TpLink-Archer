# Primera respuesta activa del firmware — quinta etapa

## Resultado real

La consulta `GET_FEATURE`, parte 0, obtuvo una respuesta válida del firmware del USB `37ad:0104` en macOS ARM:

| Campo | Valor |
| --- | --- |
| Flujos RX / TX | 2 / 2 |
| Antenas RX / TX | 2 / 2 |
| Código de protocolo | 4: 802.11be según `fw.h` |
| Código de ancho de banda | 7, sin interpretación adicional |
| Máscara de bandas | 15, sin atribuir bandas comerciales específicas |
| Secuencia C2H | 0 |

Respuesta de 16 bytes, en palabras little-endian: `07020403 0f040402 5433190e 0f020270`.

Esto verifica un intercambio activo por **buzón de registros** H2C/C2H. Todavía no valida el transporte de comandos bulk, escaneo, asociación ni conexión IP. Tampoco identifica la versión del firmware en ejecución.

## Hallazgo que permitió la consulta

Los registros base se podían leer, pero buzones y bloques MAC devolvían `0xEAEAEAEA`. `HCI_OPT_CTRL` (`0x74`) tenía el bit de habilitación WLAN IO apagado. El código Linux `rtw8922a_pwr_on_func` activa ese bit 28 y espera el bit de estado 31 antes de acceder al bloque.

Se implementó una ventana temporal que:

1. Comprueba ID de hardware, sistema activo (`0x220`) y estado de firmware 3.
2. Conserva el valor de `HCI_OPT_CTRL`, habilita únicamente WLAN IO y espera su estado con intentos acotados.
3. Rechaza buzones ocupados o valores de control inesperados.
4. Escribe `GET_FEATURE` parte 0 en `0x7140..0x714c`, actualiza el contador y dispara `0x7160`.
5. Espera la respuesta en `0x7164`, lee `0x7150..0x715c`, valida ID y longitud, y reconoce la respuesta.
6. Restaura el bit de habilitación si lo cambió, incluso ante errores. Conserva otros bits leídos durante la restauración y verifica el bit restaurado.

En la prueba real, `HCI_OPT_CTRL` comenzó y terminó en `0x001004fd`; durante la ventana pasó a `0x901004fd`. Los controles de buzón pasaron a ser accesibles y el firmware respondió tras un ciclo de espera. El contador host avanzó de `0x00` a `0x11`, como parte del protocolo; no se revierte ese contador.

No se cambió la configuración USB, se cargó firmware, se programó efuse/EEPROM ni se envió tráfico inalámbrico. La operación sí escribe registros volátiles del acceso WLAN y del buzón.

## Código y reproducción

`include/archer/register_mailbox.hpp` contiene el protocolo con bus inyectable y validación de estado. `src/macos_mailbox.cpp` lo transporta mediante solicitudes USB vendor con timeout, registrando cada lectura y escritura. No expone una consola de escritura arbitraria.

```sh
make test mailbox-universal
./build/archer-mailbox-universal --inspect
./build/archer-mailbox-universal --query-with-io-window
```

`--inspect` solo lee. `--query-phy-cap` exige que el acceso WLAN ya esté habilitado. `--query-with-io-window` lo habilita temporalmente y verifica la restauración de su bit. Los fallos devuelven 2 y se detienen; no hay resets ni reintentos de comandos automáticos.

Si ya existe una respuesta pendiente, la herramienta no la sobrescribe. Si la respuesta recibida no tiene el ID o longitud esperados, la deja pendiente y reporta error. Un fallo de restauración se informa como tal y no se oculta como éxito de consulta. Los intentos de polling están limitados a 20 por fase; cada transferencia tiene timeout de 200 ms y cada pausa es de 10 ms.

## Evidencia y pruebas

- `mailbox-observation.json`: resumen, hashes y valores de acceso WLAN inicial/final.
- `artifacts/mailbox-inspection-stage5.jsonl`: lecturas previas, con el acceso WLAN apagado.
- `artifacts/mailbox-query-stage5.jsonl`: traza completa del intercambio real; no se usó la salida de un simulador.
- `tests/fixtures/phy-cap-part0.bin`: respuesta real de 16 bytes utilizada también por las pruebas.

El test de bus simulado comprueba rechazo previo sin escrituras, timeout acotado, respuesta inesperada, envoltura de contadores, restauración ante fallo, preservación de bits ajenos y reporte de fallo de restauración. También decodifica el fixture real. Las cuatro suites C++ y nueve pruebas Python pasaron; el test de buzón pasó AddressSanitizer y UndefinedBehaviorSanitizer. Compilación universal Intel/ARM correcta, ejecución física solo en ARM.

## Fuentes y límite de REA

La consulta y el mapa de registros proceden de los snapshots Linux `mac.c:rtw89_mac_read_phycap`, `fw.c:rtw89_fw_write_h2c_reg/rtw89_fw_read_c2h_reg`, `rtw8922a.c` y `reg.h`. Los campos de respuesta están en `fw.h`. Sus copias y hashes se conservan en el manifiesto de artefactos.

REA sigue conectado, pero Hopper mostró “Demo period has expired. Document will be closed.” No se intentó eludir esa limitación. Esta etapa usa la evidencia REA previamente exportada y las fuentes Linux; no atribuye nuevos resultados de descompilación a REA. El último `binary_session` consultado informó `open:false` y `cleanup_pending:false`.

Pendiente: identificar la versión en ejecución, validar el transporte bulk con el acceso WLAN habilitado y desarrollar la inicialización MAC/PHY y gestión 802.11 antes de integrar DriverKit. La parte 1 de capacidades no se consulta automáticamente: Linux también condiciona su uso a características de versión de firmware todavía desconocidas aquí.

## Stage 6: bulk RX while WLAN IO is enabled

`archer-receive --with-io-window NEW_DIRECTORY` now keeps the verified access
window open around a PHY_CAP register query and one bounded read on each bulk IN
endpoint. The register transport is shared with the mailbox executable. The
window restores the access bit after callback exceptions as well as success;
unit tests exercise both paths. Reads consume queued data; no bulk OUT is sent.

The physical ARM test returned the same valid PHY_CAP response. Endpoints 0x84
and 0x88 each timed out with status -536854447 and zero captured bytes. Register
0x74 returned to 0x001004fd. Host mailbox counter advanced from 0x11 to 0x22 as
required by the successful register exchange. Evidence hashes and endpoint
results are in `bulk-io-window-observation.json`.

This rules out a disabled WLAN IO gate as the sole explanation for the earlier
idle RX result. It does not establish that bulk RX is broken or initialized.
Next work must identify the required USB/MAC queue initialization and a supported
H2C diagnostic command before claiming bulk H2C/C2H communication.
