# Etapa73 — extensión WDINFO TX BE v2

2026-10-29. Avance50 %. Implementación y comprobación offline; sin acceso USB.

Se amplía `tools/be_tx_descriptor.py` con `encode_info` y `encode_descriptor`.
La extensión ocupa32 bytes, detrás del cuerpo de32. El bit WDINFO_EN del
cuerpo se activa únicamente cuando se incluye realmente la extensión.
El modo anterior de cuerpo solo sigue disponible.

Campos: puerto, STBC/LDPC, fallback de datos, límite/selección de reintentos,
agregación, tasa mínima de reintento, etiqueta SW_DEFINE, densidad AMPDU y
solicitud de reporte. Se exige is_bmc explícito0/1; RTS_EN=!is_bmc y
HW_RTS_EN=1 reproducen core.c. No se habilita cifrado ni se acepta sec_en.
Los bits restantes quedan0. La etiqueta SW_DEFINE es de4 bits y no sustituye
a la secuencia Wi-Fi de12 bits; aún falta gestionar su vida/correlación.

Fuente: core.c:1787-1868 y txrx.h:248-313, mismo commit y hashes de etapa72.
La etiqueta pide un reporte, pero no implementa su recepción ni demuestra ACK.
Que una transferencia USB termine tampoco probará éxito por radio.

```
python3 tools/be_tx_descriptor.py artifacts/tx-body-stage73.json --wdinfo-json artifacts/tx-info-stage73.json
```

La salida distingue cuerpo32, extensión32, descriptor64 y bytes hex completos.
Mantiene hardware_validated=false/transmission_ready=false. El vector CLI
retenido en `artifacts/tx-descriptor-stage73.json` es sintético, sin envío.

Validación: vector little endian esperado de WDINFO, acoplamiento del bit
de presencia con longitud64, RTS para unicast/BMC, límites de cada campo y
rechazo de cifrado/modos ambiguos. `make test` pasa:74 tests Python y suites
C++ existentes. Log `artifacts/tx-descriptor-tests-stage73.log`.

Sigue pendiente comprobar cola/endpoint, encapsulado USB/checksum/agregación,
contexto MAC/puerto y parser/correlación de reportes TX. La potencia y calidad
RF conservan los límites de etapas anteriores. No hay nueva transmisión ni
asociación; el progreso ponderado permanece50 %.
