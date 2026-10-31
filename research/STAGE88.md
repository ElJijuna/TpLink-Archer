# Etapa88 — baseline IFS-CLM pasiva con intervalo válido

2026-10-31. Mac ARM. Avance52 %. Sin nuevo H2C ni trama de red.

Se incorporó el monitor IFS-CLM según la fuente Linux fijada:
`phy.c:6270–6350` unidades/periodo,6478 inicialización,6538 disparo,
6749 DONE/resultados y6974 ajuste; `phy_be.c` ccx_regs_be_v1;
`reg.h` mapas BE4 y `core.h` EDCCA_BW20_0=0.

Perfil fijo PHY0/canal9:2000ms, periodo62500, unidad32us (índice3).
Umbrales IFS64/256/1024/4096us, equivalentes2/8/32/128 unidades.
Se guardan seis registros de control; se verifica cada escritura y se
restauran incluso si falla el cuerpo. Disparo CLR0,TRIG0,CLR1,TRIG1;
se exige DONE inicialmente0 y después1 al cumplirse el intervalo.
Los contadores previos se descartan; solo se restauran controles, no datos
históricos ni una medición ajena. Se rechaza un monitor que pueda estar activo.

Prueba física `archer-ifs-clm --passive-channel9-baseline`:

| Ventana | Lectura tras ms | IFS totales | TX unidades | CCK CCA unidades | OFDM CCA unidades |
|---|---:|---:|---:|---:|---:|
|0|2025|991|0|5759|45|
|1|2022|1190|0|8947|240|

La medición terminó y respondió a actividad del entorno sin TX observado
por el contador en estas ventanas pasivas. No es una medición retrospectiva
 del intento86 ni prueba externa de calidad RF.

Controles restaurados, contexto CLEAN, canal9, TX pausado y206 secuencias.
Tests de fallo después de cada escritura setup/disparo confirman restauración;
se rechaza scheduler activo y se comprueba decodificación DONE y campos.
`make test`:90 tests Python y suites C++. Compilación universal correcta;
Intel sin prueba física.

[ifs-clm-stage88-observation.json](ifs-clm-stage88-observation.json) conserva
ventanas y hashes. Binario/logs en `artifacts/ifs-clm-passive-stage88/`.
Este monitor habilita instrumentar una prueba posterior; no la realiza.
