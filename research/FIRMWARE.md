# Firmware e inicialización — tercera etapa

## Imágenes vinculadas por código

REA/Hopper analizó el mismo SYS Win11 de SHA-256 `68f735cc46b25ce983997d7f8693b24ea75af3abaf42a2f4d69fb63ad5590332`.

El dossier `ev_1de042857caf0af0d8ade38446fff4c936e5ded230049fcf107ef9866e0ed510`, función `0x1403deaa0`, muestra referencias a dos buffers y a sus longitudes. Las instrucciones ensambladas corroboran las direcciones del pseudocódigo.

| VA buffer | VA longitud | Offset archivo | Bytes | Versión | Byte inicial |
| --- | --- | --- | --- | --- | --- |
| `0x1405a4870` | `0x1405a4860` | `0x5a3070` | 317576 | 0.34.40.0 | 1 |
| `0x1405f2100` | `0x1405a4864` | `0x5f0900` | 400248 | 0.35.47.15 | 2 |

La rama distingue un campo cero/no cero de contexto (`+0xc1` desde un puntero de contexto). No se ha demostrado la relación completa de ese campo con la revisión física; la herramienta extrae ambas imágenes sin seleccionar una para el dispositivo. Los nombres `cut1/cut2` representan el byte de la cabecera y no una certificación de compatibilidad.

`firmware-manifest.json` conserva los hashes y las secciones de ambas imágenes. Los bytes extraídos están en `artifacts/firmware/`, excluidos de Git. No se distribuyen como firmware libre.

## Validación implementada

El inspector acepta cabeceras RTL8922A v1, comprueba límites, tabla de secciones, longitud dinámica, anchura de direcciones y suma exacta del tamaño. Cuenta ocho bytes adicionales cuando la bandera de checksum lo exige. Rechaza variantes con arrays MSSC no soportados. Conserva flags sin atribuirles comportamiento no recuperado.

Para estas imágenes: cabeceras de 192/176 bytes, extensión dinámica de 80 bytes y tamaño de parte indicado de 2020 bytes. La primera tiene cuatro secciones y la segunda tres. La validación estructural no verifica firmas, checksums ni la compatibilidad con la placa.

El extractor exige el hash exacto del SYS analizado, obtiene tamaños desde los globals referenciados y traduce las VA usando las secciones PE. Un paquete distinto requiere un nuevo análisis REA; no se aplican offsets ciegamente. El directorio de salida debe ser nuevo.

## Estado observado del adaptador

`--read-startup-state` realizó tres solicitudes vendor IN, de cuatro bytes cada una:

| Registro | Valor | Campo |
| --- | --- | --- |
| `0xf0` SYS_CFG1 | `0x0c431d37` | bits 15:12 = 1 |
| `0xfc` SYS_CHIPINFO | `0x80000371` | byte bajo = 0x71 |
| `0x1e0` WCPU_FW_CTRL | `0x0c000000` | bits 29:26 = 3 |

En `rtw89-mac_be.c`, `fwdl_status_map[3]` corresponde a `RTW89_FWDL_WCPU_FW_INIT_RDY`. Es una inferencia según esa implementación Linux, sin identificación del firmware que está ejecutando este dispositivo. No se realizó carga de firmware ni ninguna escritura de registro.

## Inicialización pendiente

La referencia `rtw8922a_pwr_on_func` de Linux incluye operaciones read-modify-write de energía, polling con timeout, configuración de reloj y acceso indirecto XTAL, antes de habilitar DMAC/CMAC. Tiene ramas por transporte y revisión. No basta copiar la lista de registros: hay que portar las primitivas indirectas y recuperar los prerrequisitos del firmware y datos de placa.

Próximo paso: recuperar la consulta de versión del firmware y el canal H2C/C2H, contrastar los estados iniciales y las tablas de colas USB del Windows driver. Solo después integrar la secuencia de inicialización con cancelación, timeout y recuperación de desconexión. No se ha validado todavía una transición de energía ni una recuperación por reconexión.

## Reproducir

```sh
python3 tools/firmware.py extract-windows artifacts/windows/app/Driver/Win11/rtwlanu7.sys
python3 tools/firmware.py extract-windows artifacts/windows/app/Driver/Win11/rtwlanu7.sys --output-dir /tmp/archer-firmware-new
python3 tools/firmware.py inspect /tmp/archer-firmware-new/rtl8922a-cut2-0.35.47.15.bin
make universal
./build/archer-probe-universal --read-startup-state
make test
```

Fuentes Linux consultadas: `drivers/net/wireless/realtek/rtw89/{fw.c,fw.h,reg.h,mac_be.c,rtw8922a.c}` en torvalds/linux master. Sus snapshots y hashes se conservan en el manifiesto local; son referencias de investigación y se debe fijar commit antes de una adaptación distribuida. Evidencia REA completa de esta etapa: `rea-firmware-evidence.json` (8 registros, límites de Hopper incluidos).
