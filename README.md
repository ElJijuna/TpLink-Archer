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
