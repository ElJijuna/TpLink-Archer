# Etapa72 — codificador offline del cuerpo TX BE v2

2026-10-29. Avance50 %. No se envían tramas ni se toca hardware en esta etapa.

`tools/be_tx_descriptor.py` codifica un cuerpo TX de32 bytes little endian
para el formato BE v2 seleccionado por rtw8922a.c. Implementa campos de DMA,
longitud, cabecera, QSEL, MACID, secuencia y selección de tasa. Los tamaños
y desplazamientos proceden de core.c/txrx.h del commit Linux
c03c1a14507dfaf8b856c2608f9ac216021fc222. Identidades de los archivos locales
en [tx-descriptor-stage72-sources.json](tx-descriptor-stage72-sources.json).

La fuente rtw8922a.c:3170,3327-3328 selecciona fill_txdesc_v2 y cuerpos/info
de32 bytes cada uno. core.c:1689-1868 construye sus palabras; usb.c:393-434
selecciona el camino de red frente a firmware y activa STF_MODE en bit10.
El codificador usa STF_MODE1 por defecto, sin asumir que sea suficiente para
transmitir. Los campos opcionales no soportados permanecen0.

Se exige longitud no vacía y canal DMA explícito; se rechaza canal12 porque
los comandos firmware utilizan otro descriptor. También se rechazan campos
desconocidos, booleanos, números negativos y valores que exceden su máscara:
no se truncan silenciosamente. Las validaciones son de formato, no una lista
de canales DMA/QSEL/tasas semánticamente habilitados en este dispositivo.

```
python3 tools/be_tx_descriptor.py artifacts/tx-fields.json
```

La entrada JSON contiene campos; la salida contiene bytes hex y palabras,
con hardware_validated=false y transmission_ready=false. No exporta una trama
lista para enviar ni abre USB. El vector de prueba usa longitud24, DMA8,
QSEL18 y tasa4 únicamente para comprobar bits: no valida esos parámetros RF.

Pendientes antes de TX: WDINFO/tasa/reintentos/reportes, checksum y agregación
USB cuando corresponda, selección de endpoint/cola, contexto MAC/puerto y
confirmación correlacionada. También siguen pendientes semánticas DPK/TSSI
y validación de potencia/calidad RF. No se habilita scheduler TX.

Tres tests nuevos comprueban un vector de bytes esperado, límites de todos
los campos, rechazo de caminos no soportados y separación de MACID/secuencia.
`make test` pasa:72 tests Python más las suites C++ existentes.
Log `artifacts/tx-descriptor-tests-stage72.log`. Avance global sin incremento
porque aún no hay transmisión física ni asociación.
