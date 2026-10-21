# CMAC0 y calibraciones de transmisión: prueba física

Fecha del cliente: 2026-10-21. Equipo probado: Mac Apple Silicon, adaptador
37ad:0104/RTL8922A CV1, AID0x6933, RFE1, USB2 a480Mbps.

La repetición final completó tres procesos con código0: reinicio y carga de
firmware con inicialización MAC/radio, RFK inicial y RFK de transmisión.
Es una recarga de MCU desde el estado alimentado conocido; no es una prueba
de arranque desde una reconexión USB. Se usaron principal0.35.41.0 y BBMCU0
0.1.0.0 vinculados por hash. No se obtuvo una lectura independiente de versión
residente. Los binarios ejecutados quedaron preservados antes de usarlos.

Manifiesto con hashes y resultados:
[cmac-tx-rfk-stage42-observation.json](cmac-tx-rfk-stage42-observation.json).

| Etapa | Resultado observado |
| --- | --- |
| CMAC0 | Rama completa `cmac_init_be` para CV1/USB2/SCC; sondeos CAM completados |
| DLE/HFC/DMAC | Partición SCC y secuencias de inicialización completadas |
| BB/AFE y rutas | 12 escrituras sethw, 55 de rutas A/B de dos streams, 133 de canal/ganancia |
| Placa/potencia | Trims PA/PAD reales y 197 escrituras de potencia del perfil diagnóstico WW |
| DACK inicial | Estado OK; siete flags de timeout y DACK_FAIL a cero |
| RXDCK inicial/final | Estado OK; canal1/2GHz/20MHz, timeout0 en ambas rutas |
| TXGAPK | Estado OK y `is_txgapk_ok=1` en el informe antiguo de84 bytes; sin campos failcode |
| IQK | Estado OK; flags TX/RX limpios y contexto correcto en ambas rutas |
| TSSI SCAN/NORMAL | Estados OK; NORMAL entrega informe de56 bytes, alignment_state[0,0,0,0] |
| DPK | Estado OK; contexto correcto, timeout/overflow[0,0], path_status[4,4] |

Todos los comandos pedidos se ejecutaron. **No se declara validación completa
de calidad de calibración.** El significado exacto de alignment_state TSSI y
del estado DPK4 en esta versión no está demostrado. El código distingue
finalización, flags de error conocidos y calidad desconocida.

## Secuencia reproducida

1. `artifacts/archer-radio-init-stage42 --powered-cold-scc-radio artifacts/radio-init-stage42`.
2. `artifacts/archer-rfk-stage42 --initial-rfk artifacts/rfk-stage42`.
3. `artifacts/archer-tx-calibrate-stage42 --channel1-world-calibrate artifacts/tx-rfk-stage42`.

Esos directorios ya existen: los ejecutores requieren uno nuevo por intento.
Las opciones dependen del estado previo comprobado; no son una receta de
instalación desde cualquier estado. No se debe borrar el ledger de secuencias.
Las reservas finales de TX fueron59..66: sincronización PRE_NOTIFY, TSSI_SCAN,
PRE_NOTIFY, TXGAPK, IQK, TSSI_NORMAL, DPK y RXDCK. Los informes detallados pueden
llegar después del OK y se esperan antes de avanzar. No hay reintentos automáticos.

## Correcciones y decisiones de perfil

- La secuencia completa CMAC incluye scheduler, CAM, filtros RX, NAV, spatial
  reuse, TMAC/TRXPTCL/RMAC, reserva CSI, PTCL y DMA. Estaciones, puertos, máscaras
  globales de interrupción y la interfaz de red no forman parte de esa función
  y no se presentan como implementados.
- Se añadieron los pasos PLL de `rfk_hw_init`: XTALe1 ORf8, d1 AND~60 y16 OR38.
  También se integró `bb_sethw`/control AFE antes de configurar las rutas.
- El perfil de investigación fuerza SCC y noDBCC; su modo MLO coherente es1
  (`MLO_DBCC_NOT_SUPPORT`). No se infiere del arranque genérico BE, que habilita
  DBCC en una rama posterior. Las pruebas anteriores con PRE_NOTIFY0x2002
  permanecen conservadas, pero no justifican ese modo como configuración BB
  aceptada: `ctrl_mlo` lo rechaza. La repetición final usa1 en BB y firmware.
- Se verifica el sintetizador A seleccionado por la fuente para PHY0 y los
  dos registros digitales de canal. RF18 analógico B sigue leyendo0 tras la
  escritura solicitada; la fuente no impone igualdad allí. Se registra esa
  diferencia y no se inventa que el sintetizador B esté apagado. Los informes
  IQK/DPK/RXDCK confirman el contexto de ambas rutas, no una medición externa
  de frecuencia.
- Las ganancias y trims proceden del firmware/efuse capturados, con hashes.
  Los ceros de TSSI trim son estado inicial de software probado mediante la
  cadena de asignación `ieee80211_alloc_hw`→`wiphy_new_nm`→`kzalloc`; no se
  presentan como calibración de fábrica. El mismo criterio se aplica a celdas
  de ganancia/potencia omitidas que el código deja a cero.
- Potencia: perfil explícito WW, SAR NONE y ganancia de antena sin configurar;
  conserva las tablas reales del firmware. No equivale a seleccionar el país
  del usuario. El scheduler de red permanece pausado al terminar o fallar.
- BTC se mantiene deliberadamente sin inicializar en el host de investigación.
  El código fuente omite la política hardware bajo `!init_ok`; esto no implementa
  coexistencia de producción ni demuestra ausencia de interferencia Bluetooth.

## Compatibilidad de informes y límites de calidad

El firmware usa TXGAPK84 e IQK184, frente a100/516 bytes de la fuente reciente.
Ambos formatos antiguos están definidos en una cabecera primaria histórica;
se conservan los bytes originales y los campos nuevos ausentes quedan ausentes.

La primera comprobación DPK exigía `path_ok==1` sin respaldo y se corrigió.
La fuente RTL8922A solo imprime ese byte. Un chip relacionado usa un bitmap;
los logs físicos de A muestran conjunto K2 y máscara4, lo que corrobora esa
hipótesis, pero no demuestra el productor exacto RTL8922A ni el significado
de todos sus valores. Por eso `error_flags_clear()` y `path_success()` son
conceptos separados y este último sigue desconocido para[4,4]. No se transforma
un desconocido en éxito ni se etiqueta4 como fallo.

Los campos TSSI se conservan sin inventar una función de éxito: la fuente
primaria solo los imprime. No se midieron EVM, espectro ni potencia externa.
La ruta experimental de carga USB mantiene la omisión explícita DCPU0x888
ya documentada; no es una solución general para otras variantes.

## Código, pruebas y evidencia

- CMAC: `include/archer/cmac_init.hpp` y `src/macos_cmac_init.cpp`.
- Canal/AFE/placa: `channel_bus.hpp`, `channel_power.hpp`, `trx_path.hpp` y `tools/channel_bundle.py`.
- RFK TX: `tx_rfk.hpp`, `src/macos_tx_calibrate.cpp` y `tools/tx_calibration_bundle.py`.
- Detalles de fuente: `CMAC_INIT.md`, `TRX_PATH.md`, `CHANNEL_POWER.md`, `TX_RFK.md`.
- Fuentes adicionales fijadas: `tx-source-pin-stage42.json`.

`make test` pasa34 suites C++ y46 pruebas Python. Las herramientas nativas
compilan arm64/x86_64; solo se probaron físicamente en ARM. Los fixtures
stage42 son copias exactas de transferencias C2H de calibración, sin tráfico
del aire. `tx_rfk_physical_test.cpp` reproduce los resultados y preserva los
desconocidos de DPK/TSSI. Las capturas completas permanecen en `artifacts/`,
fuera del seguimiento Git; el manifiesto conserva hashes y procedencia.

La siguiente validación pendiente es establecer las semánticas DPK/TSSI o
contrastar calidad RF con una referencia fiable. El controlador todavía no
expone una interfaz Wi-Fi ni realiza asociación; Linux e Intel físico siguen
pendientes.
