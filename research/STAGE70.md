# Etapa70 — informe conjunto de capturas pasivas

2026-10-29. Avance50 %. Consolidación offline; no nuevo escaneo físico.

`tools/passive_sweep_report.py` reúne los dwells de recepción1/6/11 por
BSSID+SSID en bytes. Conserva cada observación agregada por canal recibido
y canal DS anunciado, incluidos sus conteos y ordinales locales. Un SSID
oculto no se fusiona por suposición con otro nombre del mismo BSSID.
Los registros resultantes no equivalen a un conteo de equipos físicos.

Antes de emitir el informe ejecuta el decoder existente sobre cada captura
y exige igualdad JSON con networks.json. Rechaza canales incompletos,
sondas activas, errores FCS, overflow, duplicados dentro de un dwell y
conteos de gestión inconsistentes. Registra hashes del decoder y de los
tres JSON fuente. No verifica por sí mismo la terminación del ejecutor USB,
RFK ni el retorno de canal: esas garantías siguen en la etapa68.

Validación con `artifacts/passive-retune-stage68c`: **45 BSSID distintos y
45 pares BSSID/SSID**. Los tres canales coinciden con su replay offline.
Resultado local: `artifacts/passive-sweep-report-stage70.json`.
No se publican aquí SSID ni identificadores de redes vecinas.

```
python3 tools/passive_sweep_report.py artifacts/passive-retune-stage68c > artifacts/sweep-report.json
```

`make test` pasa:67 tests Python, además de las suites C++ existentes.
Los tres tests nuevos comprueban procedencia de canal recibido frente a DS,
SSID binario/oculto, conservación de entradas e invalidación de capturas
incompletas o inconsistentes. Log `artifacts/passive-report-tests-stage70.log`.

Pendiente integrar el barrido en el coordinador de recuperación y repetir
todo el flujo desde el estado actual del adaptador. No se ha cargado firmware,
retuneado ni transmitido en esta etapa. TX, asociación e interfaz de red
continúan pendientes. El LED permanece validado por la etapa69.
