# Archer USB — investigación y base de controlador

## Checklist resumido — avance estimado: **40 %**

Actualizado: **2026-10-22**, etapa54. Objetivo: controlador Wi-Fi funcional
para macOS ARM/Intel, con Linux posteriormente.

- [x] Identificación del dispositivo y protocolo USB/registros — **10/10**.
- [x] Carga experimental de firmware principal y BBMCU0 — **10/10**.
- [x] DLE/HFC/DMAC/CMAC0 y tablas BB/radio/NCTL en USB2/SCC sobre Mac ARM — **15/15**.
- [ ] Calibraciones y validación RF completas — **5/10**: comandos ejecutados; faltan semánticas DPK/TSSI y contraste de calidad RF.
- [ ] Ruta completa de datos Wi-Fi TX/RX — **0/15**.
- [ ] Escaneo, asociación y seguridad Wi-Fi — **0/15**.
- [ ] Integración macOS: DriverKit, interfaz de red, instalación y ciclo de vida — **0/15**.
- [ ] Validación física en Mac Intel — **0/5**.
- [ ] Implementación y validación en Linux — **0/5**.

**40/100 puntos de hitos ponderados.** Es una estimación de alcance completado,
no del tiempo restante. Compilar para Intel no equivale a validarlo físicamente.
El adaptador todavía no ofrece conexión Wi-Fi utilizable.

**Estado actual:** CMAC0 completo para el perfil USB2/SCC, DLE/HFC/DMAC,
tablas de radio y secuencia TXGAPK/IQK/TSSI/DPK ejecutados en el Mac ARM.
La repetición desde firmware recargado terminó con respuestas OK de todos
los comandos y RXDCK final limpio. IQK y TXGAPK reportan éxito; DPK no muestra
timeouts/overflow, pero su máscara de estado y los estados de alineación TSSI
siguen sin interpretación definitiva. No se afirma calidad RF plenamente validada.
TX de red queda pausado; todavía no hay escaneo, asociación ni interfaz de red.
Los ejecutables compilan para Intel/ARM; falta prueba física en Intel y Linux.
Evidencia y límites: [research/STAGE42.md](research/STAGE42.md).

Etapa43: el decodificador RX ya expone errores CRC/ICV, tasa, ancho de banda
y conversión de cabecera. Una captura anterior contiene un paquete normal
de 124 bytes sin errores de descriptor y con cabecera sin convertir. Esto
valida el análisis de esa captura; falta recepción reproducible con CMAC
actual, validación de tramas y escaneo pasivo. El avance permanece en **40 %**.
Detalles: [research/STAGE43.md](research/STAGE43.md).

Etapa44: parser offline de beacons/probe-responses con límites de cabecera e
información variable, probado con tramas sintéticas. FCS requiere política
explícita; todavía falta comprobar recepción pasiva con CMAC actual.
Detalles y uso: [research/STAGE44.md](research/STAGE44.md).

Etapa45: ventana RX probada físicamente durante cinco segundos en ARM:
filtros y motor de respuestas restaurados, TX pausado, **0 transferencias**.
Falta localizar el bloqueo del recorrido RX; avance **40 %**.
Evidencia: [research/STAGE45.md](research/STAGE45.md).

Etapa46: contadores PHY reiniciados y medidos físicamente: **CCK376,
OFDM31 y HT5 paquetes con CRC correcto** durante cinco segundos, pero
**0 transferencias USB**. La radio decodifica tráfico; falta aislar el bloqueo
posterior en RMAC/DLE/despacho/USB. Avance **40 %**.
Evidencia: [research/STAGE46.md](research/STAGE46.md).

Etapa47: PLE grupo0 vacío y cuota RX sin uso en las muestras. Activar
temporalmente RMAC CH_EN tampoco entregó paquetes USB; se restauró el bit.
Continúa el diagnóstico de filtros y entrega PHY→RMAC. Avance **40 %**.
Evidencia: [research/STAGE47.md](research/STAGE47.md).

Etapa48: contrastados A_BC, SNIFFER_MODE y motor de respuestas por separado;
ninguno liberó RX USB. Controles BB consultados no muestran PMAC TX/RX grant
activo ni reset asíncrono retenido. Se conserva evidencia y avance **40 %**.
Detalles: [research/STAGE48.md](research/STAGE48.md).

Etapa49: se contrastó PHY report de ocho bytes siguiendo la fuente; campos
aplicados y restaurados, todavía **0 transferencias USB**. Se identificó
inicialización PHY-status pendiente de portar. Avance **40 %**.
Detalles: [research/STAGE49.md](research/STAGE49.md).

Etapa50: inicialización PHY-status no-monitor portada y probada en ventana
reversible. Se restauraron19 controles; PHY contó CRC OK, pero USB continúa
en **0 transferencias**. Falta aislar el contexto/entrega RMAC. Avance **40 %**.
Detalles: [research/STAGE50.md](research/STAGE50.md).

Etapa51: puerto0 sin enlace contrastado y restaurado; todavía RX USB0.
Codificador CREATE/REMOVE de rol estación implementado y probado offline.
Falta la transacción completa de interfaz: JOIN, CAM, tablas y confirmaciones.
Avance **40 %**. Detalles: [research/STAGE51.md](research/STAGE51.md).

Etapa52: JOIN_INFO BE y CAM V0 de60 bytes implementados y probados offline,
con invalidación CAM y correlación de ACK. No se enviaron al adaptador;
faltan tablas DMAC/CMAC y ejecución completa con limpieza. Avance **40 %**.
Detalles: [research/STAGE52.md](research/STAGE52.md).

Etapa53: tablas por MACID CMAC G7/DMAC V2 y pausa/sueño individual
implementadas y probadas offline, con máscaras y políticas ACK de la fuente.
Falta coordinarlas y validarlas físicamente. Avance **40 %**.
Detalles: [research/STAGE53.md](research/STAGE53.md).

Etapa54: coordinador del contexto estación implementado y probado offline:
orden de nueve comandos, políticas ACK, retirada controlada y bloqueo ante
resultado incierto. Falta ejecutor USB y validación física. Avance **40 %**.
Detalles: [research/STAGE54.md](research/STAGE54.md).


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

### Identidad analógica validada

El lector XTAL-SI obtuvo AID 0x6933 y revisión analógica 1. La selección
offline ya filtra tablas por esa identidad y conserva sus índices de radio.
`make identity-universal` construye el lector Intel/ARM. Falta implementar
el intérprete de condiciones de tablas antes de aplicar registros RF.

### Intérprete PHY offline

`tools/phy_table.py` resuelve condiciones y retardos para RFE 1/CV 1. Sus
selecciones coinciden, par por par y en orden, con las funciones originales
Linux ejecutadas sin hardware. El plan todavía no escribe registros RF ni
inicializa la radio. Véase `research/phy-table-reference-check.json`.

### Ganancias BE y acceso RF-v2

`tools/bb_gain.py` decodifica las ganancias estructuradas; los arrays coinciden
con los callbacks originales Linux en 1512 posiciones. El helper C++
`include/archer/rf_access.hpp` calcula direcciones y comandos RF-v2/HWSI sin
ejecutarlos. La inicialización física BB/RF sigue pendiente.

### Estado físico BB y backend RF

La prueba temporal ya permite leer los registros BB sin valores de acceso
inválido. La consulta PHY_CAP y la identidad AID 0x6933 responden. La lectura
RF por HWSI aún agota el plazo: ni los controles de la tabla seleccionada ni
el reloj BB de 640 MHz bastaron para resolverla. Todos los controles
temporales se restauraron con lectura de verificación.

`make bb-probe-universal mailbox-state-universal` construye las herramientas.
`archer-bb-probe-universal --inspect-prerequisites` obtiene una captura fija
de controles dentro de la ventana BB temporal, sin lanzar comandos RF. Las
opciones `--table-hwsi-control` y `--clock-hwsi-control` son experimentos
explícitos, con valores iniciales restringidos y restauración. No constituyen
una inicialización BB/RF completa.

La lectura independiente de controles confirma BBMCU0 en reset y boot-ready
desactivado. Las fuentes y el archivo firmware ya tienen commits fijados; la
versión residente sigue desconocida. El siguiente paso es implementar una
secuencia explícita de arranque/carga, incluyendo BBMCU.
Resultados: `research/bringup-stage23-observation.json` y
`research/hwsi-stage24-observation.json`. Aún no hay escaneo ni conexión Wi-Fi.

### Inspección de firmware y BBMCU

`make firmware-state-universal` construye un inspector de cuatro registros,
sin escrituras ni comandos de buzón. Ejecuta
`./build/archer-firmware-state-universal` desde la raíz del proyecto.
`python3 tools/bbmcu.py artifacts/rtw8922a_fw-1.bin 1` selecciona y valida
estructuralmente los candidatos del archivo para CV=1: principal 0.35.41.0 y
BBMCU0 0.1.0.0. No los carga ni los identifica como firmware residente.
Véase `research/firmware-state-stage25-observation.json`.

### Recuperación después de reconectar

Estado actual: el dispositivo volvió a Wi-Fi `37ad:0104`; la causa exacta
del cambio no quedó establecida. Los párrafos siguientes registran los
intentos previos en modo instalador.

El adaptador reapareció como CD virtual Realtek `0bda:1a2b` (DISK), no como
Wi-Fi `37ad:0104`. Los helpers experimentales de expulsión compilan, pero
macOS bloqueó el acceso antes de enviar comandos. Se solicitó expulsión con
permisos de administrador del CD virtual identificado. No se ha validado aún
el cambio de modo; véase `research/reconnect-mode-observation.json`.

Si `diskutil` no encuentra el CD virtual, el helper experimental
`build/archer-optical-eject-universal --inspect` lo redescubre por IORegistry.
Su opción `--eject-installer` requiere sudo y usa DKIOCEJECT sobre el medio
Realtek verificado. La inspección está validada; la expulsión administrativa
todavía no se ha ejecutado ni ha confirmado el retorno al modo Wi-Fi.

La expulsión óptica fue aceptada, pero el adaptador volvió a DISK. El helper
USB directo ahora descubre los endpoints reales 0x84/0x05; aún falta probar
su apertura desde Terminal con sudo. Un status 0 de expulsión no confirma
el cambio de modo: debe reaparecer `37ad:0104`.

### Preparación de carga y trabajo pendiente

`./build/archer-efuse-universal --security-only NUEVO_DIRECTORIO` lee solo
cuatro bytes de selección de seguridad efuse, sin programación OTP. El
resultado físico actual es `ff ff ff ff`. El preparador offline se reproduce:

```sh
python3 tools/firmware_download.py artifacts/rtw8922a_fw-1.bin 1 artifacts/efuse-security-stage26/firmware-security.bin
```

Genera metadatos y hashes de 193 paquetes principales y 19 BBMCU; no reinicia
CPU ni envía firmware. El estado completo y criterios pendientes están en
[research/ROADMAP.md](research/ROADMAP.md).

### Ejecutor experimental de arranque

`make firmware-boot-universal` genera un bundle ligado al hash del firmware y
compila el ejecutor Intel/ARM. `--boot-and-read-rf` intenta reiniciar CPU y
cargar principal+BBMCU0 sobre el transporte USB ya preparado; todavía no es
un inicializador general MAC/DLE. El primer intento se detuvo en el acceso
DCPU 0x888, después del reset y antes de enviar firmware. Se requiere
recuperar el dispositivo antes de otro intento. El ejecutor actualizado
comprueba ese acceso antes de cualquier reset.

`archer-firmware-state-universal --inspect-boot-access` inspecciona los anchos
de acceso DCPU y controles fijos de transporte cuando el firmware original
está listo. Evidencia: `research/firmware-boot-stage27-observation.json`.
El hito de firmware/BBMCU y respuesta RF aún no está completado.

### Verificación del hito de firmware/RF

`make firmware-boot-universal bb-probe-universal` construye las herramientas
ligadas al firmware de referencia. El perfil experimental validado es
`./build/archer-firmware-boot-universal --boot-usb-no-dcpu`; requiere el estado
inicial USB/HAXI conocido y no reintenta cargas. Una vez cargado,
`--verify-loaded-rf` comprueba cuatro registros por ruta sin reset ni recarga.
La ruta predeterminada sigue rechazando DCPU inaccesible antes del reset.
No se debe repetir el arranque sobre un dispositivo ya inicializado: sus
precondiciones lo rechazan. Consulte las limitaciones y estado final en el
informe de etapa29; los apartados anteriores conservan la investigación.
