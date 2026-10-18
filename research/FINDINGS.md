# Hallazgos — 2026-10-18

## Observaciones

| Evidencia | Resultado | Alcance |
| --- | --- | --- |
| IORegistry y `make probe` | Realtek 802.11be WLAN Adapter, USB `37ad:0104` | Dispositivo conectado; no implica conectividad |
| IOUSBHostInterface | Interfaz 0, clase/subclase/protocolo `ff/ff/ff`, 9 endpoints | Conteo; direcciones y tamaños aún no recuperados |
| IORegistry | `UsbLinkSpeed=480000000` | Velocidad reportada en esta conexión; no mide throughput |
| INF oficial, línea 28 | `37ad:0104` → `RTL8922au.ndi` | Coincidencia exacta de hardware ID Windows |
| REA overview `ev_35fed20bdebf6e2f11c9f2ac202a48c76e5b925a99ad698c642aff89c9ae365d` | PE x86_64; 18.062 procedimientos, 14.350 cadenas | Inventario estático Hopper; no fuente original |
| REA strings `ev_78cc7fcf9e7378167d4791dae3270b6d553a5eab8dbd42f9319e7c9bdf9ce9d2` | Referencias 8922A y `WLANFW_V0.35.47.15_8922A_TP-LINK-0-gd3a16768` | Cadena embebida; no prueba de firmware ejecutado |
| REA dossier `ev_c049cf29b619517601c800224f75a40179ffd0d71161b412f3d71fc5ce630493` | Función `0x140301420` referencia `hal_init_8922au` | Pseudocódigo parcial; no reconstruye encendido completo |

## Identidad de artefactos

- Página oficial: https://www.tp-link.com/sg/support/download/archer-tbe230u/v1/
- Paquete: https://static.tp-link.com/upload/driver/2026/202601/20260104/Archer_TBE230U_V1_TBE400U_V1_TBE3500U_V1_5002.24.126.7_Win10_Win11.zip
- SHA-256 ZIP: `8343fb20dbfe11571ab2da94856fbabb6fa2a1f25387adaa0f5eb718cf6a1b33`.
- SHA-256 instalador: `3ee1cab6e89fa196783c901235a3f214c620b392cbe69f04c1f5241915a08045`.
- SHA-256 controlador Win11: `68f735cc46b25ce983997d7f8693b24ea75af3abaf42a2f4d69fb63ad5590332`.
- Win10: versión `5002.24.126.7`; Win11: `5102.24.126.7`, ambos con fecha INF 2025-11-18.
- Extracción ZIP por REA: `ev_e4493adcec74428a9ec2e6297f2b4fc47fed4a2a0489ab7d11671363c7d873c7`. Después se extrajo Inno Setup con innoextract 1.9 sin ejecutar el instalador.

`rea-evidence.json` conserva los cinco registros de la sesión del SYS (overview, strings, búsqueda de procedimientos, xrefs y dossier), con sus limitaciones completas. El ID de extracción ZIP corresponde a una sesión anterior y se conserva aquí como referencia, no como registro incluido en ese bundle. Los resultados INF y sus hashes están en `inf-matches.json`; el manifiesto de archivos está en `artifact-manifest.json`.

## Inferencias y desconocidos

La familia seleccionada por el controlador oficial es RTL8922AU. No se ha leído aún el identificador interno del silicio ni confirmado modelo comercial, revisión de placa o chip-cut.

La referencia Linux `rtw8922au.c` consultada contiene TP-Link `37ad:0100` y `37ad:0101`, pero no `37ad:0104`. Esa búsqueda está limitada a ese archivo y snapshot, y no demuestra ausencia de soporte en todas las ramas o distribuciones. Copias locales de los archivos consultados y sus hashes se conservan en `artifacts/`.

La codificación de control existente sigue `__rtw89_usb_vendorreq` y las constantes VENQT de Linux. No se enviaron estas solicitudes al adaptador. El dossier Windows inspeccionado no confirma por sí mismo la equivalencia de todo el protocolo con Linux.

No se ha validado asociación, firmware, cifrado, tráfico USB de datos ni red. El bundle REA declara cero unknowns registrados: ese contador no significa que el protocolo esté resuelto; las preguntas pendientes están documentadas aquí y en PROPOSAL.md.

## Verificación realizada

`make test probe universal`: correcto en Apple Silicon. Tres pruebas Python, pruebas C++ de encabezados y rangos, detección real y compilación universal. Ejecución Intel, Linux y controlador DriverKit pendientes.

## Segunda etapa: descriptores y primera transferencia real

El cliente nativo IOUSBLib recuperó una configuración de 81 bytes. Interfaz 0, alternate setting 0: bulk IN 0x84/0x88 y bulk OUT 0x05/0x06/0x07/0x09/0x0A/0x0B/0x0C; todos con máximo de 512 bytes. No se han determinado todavía los roles de cada cola. El descriptor se conserva como fixture y su hash está en `hardware-observation.json`.

Una solicitud IN (`c0 05 fc 00 00 00 04 00`) obtuvo 4 bytes: valor little-endian `0x80000371`, byte bajo 0x71. Se eligió el registro por `rtw89_read_chip_ver` en Linux core.c y `R_BE_SYS_CHIPINFO` en reg.h. Es una observación runtime del registro, no una validación completa de revisión/placa ni de compatibilidad con todo el driver Linux. No se escribieron registros, no se abrió el dispositivo en exclusiva y no se cargó firmware.

El sandbox rechazó el acceso al cliente USB con -536870210; la misma consulta funcionó fuera del sandbox. La herramienta devuelve error ante acceso denegado, respuestas cortas o archivo de salida existente.

Nuevas pruebas: descriptores truncados, longitudes inválidas, endpoint duplicado, conteos incoherentes, alternate settings y fixture real. Compilación universal y ejecución ARM verificadas; prueba Intel pendiente.

El parser también pasó AddressSanitizer y UndefinedBehaviorSanitizer con el fixture y los casos malformados.

## Tercera etapa

Se recuperaron dos imágenes embebidas mediante buffers y longitudes referenciados por código, y se verificó lectura de revisión/estado de firmware. Ver `FIRMWARE.md`, `firmware-manifest.json` y `startup-observation.json`. El estado bruto 3 se interpreta como inicializado en la referencia Linux; no identifica la imagen ejecutada ni acredita conexión Wi-Fi. Las nueve pruebas Python y ambas suites C++ pasaron; compilación universal correcta.

## Cuarta etapa

Se implementaron cabeceras H2C, envoltura USB y parsing RX/C2H. Prueba real de entradas bulk: ambas respondieron por timeout sin datos durante un intento acotado, con apertura y cierre normal de la interfaz. Ver `FIRMWARE_CHANNEL.md` para evidencia, alcance del resultado y limitaciones. No se ha observado una respuesta C2H ni identificado una consulta de versión activa.
