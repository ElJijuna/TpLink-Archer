# Archer USB — investigación y base de controlador

Proyecto experimental para TP-Link USB `37ad:0104`, con destino macOS Intel/Apple Silicon y Linux posteriormente.

**Estado: diagnóstico y protocolo USB parcial. Todavía no es un controlador Wi-Fi funcional.** No instala extensiones. `--read-startup-state` consulta tres registros documentados (0xF0, 0xFC y 0x1E0). El diagnóstico básico solo consulta IORegistry; `--read-chip-info` envía una única lectura USB del registro 0xFC, sin escrituras ni carga de firmware.

## Ejecutar

Con Xcode Command Line Tools y Python 3:

```sh
make test
make probe
make universal
make receive-universal decode
./build/archer-decode tests/fixtures/synthetic-c2h-be.bin
./build/archer-probe-universal --descriptors
./build/archer-probe-universal --dump-descriptors /tmp/archer-new-config.bin
./build/archer-probe-universal --read-chip-info
./build/archer-probe-universal --read-startup-state
python3 tools/firmware.py extract-windows artifacts/windows/app/Driver/Win11/rtwlanu7.sys
python3 tools/inspect_inf.py /ruta/al/paquete/extraido
```

`probe` consulta IORegistry, muestra únicamente el adaptador objetivo y devuelve 1 si está ausente o 2 si falla la enumeración. El ejecutable universal contiene arm64 y x86_64; eso no sustituye pruebas en un Mac Intel físico.

`--descriptors` enumera los endpoints mediante IOUSBLib; `--dump-descriptors` guarda una configuración binaria en un archivo nuevo. `--read-chip-info` utiliza un timeout máximo solicitado de 1 segundo y comprueba la longitud recibida. macOS puede impedir estas operaciones dentro de un sandbox; requieren acceso USB del proceso. No usa apertura exclusiva ni cambia la configuración. Este transporte de diagnóstico IOUSBLib no sustituye la extensión DriverKit del producto final.

`register_request()` construye los ocho bytes del encabezado de control para registros, sin enviar solicitudes. Su dirección de 24 bits y anchuras 1/2/4 proceden de la implementación pública rtw89. No contiene secuencias de encendido, calibración ni firmware.

## Evidencia y propuesta

La tercera etapa recuperó dos imágenes embebidas del SYS oficial y añadió inspección estructural offline. Consultar [research/FIRMWARE.md](research/FIRMWARE.md) para hashes, reproducción y límites. No se ha cargado ninguna de estas imágenes al adaptador.

Consultar [research/FINDINGS.md](research/FINDINGS.md) y [research/PROPOSAL.md](research/PROPOSAL.md). Los paquetes originales se mantienen en `artifacts/`, excluidos de Git. No se incluye una licencia de redistribución para binarios TP-Link.

El INF oficial vincula este USB ID a `RTL8922au.ndi`. Esto identifica la familia que Windows selecciona; el modelo comercial exacto sigue sin comprobarse físicamente.

La cuarta etapa añade codificación H2C, envoltura USB BE y decodificación RX/C2H con pruebas. [research/FIRMWARE_CHANNEL.md](research/FIRMWARE_CHANNEL.md) documenta el intento de recepción real (dos timeouts, sin datos) y el comando de recepción acotada. Todavía no se envían comandos al firmware.

La quinta etapa logró una respuesta real del firmware mediante `GET_FEATURE` por buzón de registros. [research/REGISTER_MAILBOX.md](research/REGISTER_MAILBOX.md) documenta el protocolo y la prueba: 2 flujos TX/RX y 802.11be. `make mailbox-universal` compila el diagnóstico; `--query-with-io-window` escribe registros volátiles y restaura después el bit de acceso WLAN. Todavía no proporciona conexión Wi-Fi.

### Recepción bulk con ventana WLAN IO

`make receive-universal` y
`./build/archer-receive-universal --with-io-window NUEVO_DIRECTORIO` permiten
repetir la prueba acotada con restauración del acceso WLAN. La prueba física
ARM obtuvo respuesta PHY_CAP por registros, pero ambos endpoints bulk IN
agotaron su tiempo sin datos. Véase `research/bulk-io-window-observation.json`.
Todavía falta validar la inicialización y los comandos del transporte bulk.

### Estado de inicialización USB/MAC

`archer-mailbox-universal --inspect-transport` inspecciona las colas dentro de
la ventana WLAN. La prueba física confirma HCI/HAXI DMA habilitados, pero
agregación RX y CMAC deshabilitados. El mapa y las limitaciones están en
`research/TRANSPORT_INITIALIZATION.md`; no se modificaron registros DMA.

### Plan DLE USB2 sin acceso al hardware

`python3 tools/dle_plan.py` genera las cuotas y los campos del perfil normal
USB2 SCC. Valida los 589824 bytes de memoria y las cuotas H2C/C2H. El plan es
offline y todavía no ejecuta la transición del firmware ni inicializa Wi-Fi.

### Primer intercambio bulk confirmado

El adaptador recibió un LOG_CFG H2C de diagnóstico por USB y devolvió un
REC_ACK C2H coincidente en el Mac ARM. Captura real y límites en
`research/bulk-command-observation.json` y `research/FIRMWARE_CHANNEL.md`.
Se confirmó recepción del comando; todavía no hay radio, escaneo ni conexión.

El seguimiento de comandos ya validó físicamente REC_ACK y DONE_ACK exitoso
para LOG_CFG, rechazando un ACK pendiente de otra secuencia. Véase
`research/bulk-completion-observation.json`. Aún falta inicializar la radio.

### Sesiones y preparación de radio

La prueba bulk completa dos comandos y conserva las secuencias entre procesos
en `artifacts/37ad-0104-sequences.bin`. Ejecutar desde este proyecto y conservar
ese archivo: no hay reciclaje automático al agotar las 256 secuencias. Se
validaron físicamente las secuencias 2–5 con retorno 0. El decodificador efuse
offline y los requisitos pendientes están en `research/RADIO_BRINGUP.md`.
La radio todavía no se ha inicializado.

### Efuse de fábrica adquirida

`make efuse-universal` construye el lector DDV/PHY Intel y ARM. En el Mac ARM
se adquirieron ambos mapas y se verificó la restauración de sus controles.
El bloque RF contiene RFE tipo 1 y trim de cristal 0x42. Los decodificadores
son offline; esos parámetros todavía no se han aplicado a la radio. Evidencia
en `research/efuse-acquisition-evidence.json`.

### Tablas y calibración PHY identificadas

El decodificador PHY obtiene trims térmicos A=−1/B=0 y los códigos PA/PAD.
El analizador de firmware Linux localizó tablas BB, ganancias, radio A/B y
NCTL. No se ejecutaron esas tablas: falta validar el AID del chip y sus
condiciones. Consulte `research/RADIO_BRINGUP.md`, etapa 14.
