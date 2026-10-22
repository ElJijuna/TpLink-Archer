# Etapa50 — inicialización PHY-status reconstruida

Fecha: 2026-10-21. Se portó el subconjunto PHY0 no-monitor de
`__rtw89_physts_parsing_init` para RTL8922A, Linux fijado
`c03c1a14507dfaf8b856c2608f9ac216021fc222`:

- `phy.c:7065-7208`: traducción de páginas y modificación de bitmaps.
- `phy.h:274-293`: páginas0..16, reservada9, EHT16.
- `phy_be.c:165-169,1941-1950`: gen_be, base20000, configuración histograma.
- `reg.h`: bitmap inicial073c, EHT0788, histograma0738.

Se desactivan los disparos de reportes de fallo/break mediante bits3:2,
se añade IE13 a MU/TB, IE1 a TB/CCK y se retiran IE4..7 de páginas de
paquete; HT/VHT/HE/EHT añaden IE20. Se preservan los demás bits.
La página reservada9 no se toca; páginas10..15 se desplazan una posición;
EHT usa0788, no la fórmula consecutiva. RTL8922A no habilita HDR2.
No se añaden IE9/10 de monitor: esta reconstrucción usa la rama no-monitor.

`--diagnostic-physts` aplica17 controles journalados, habilita temporalmente
el informe PHY de etapa49, ejecuta la captura y restaura los19 controles
PHY-status/report, además de filtros y motor de respuestas de la ventana RX.
No implementa el resto de `rtw89_phy_dm_init`, ni contexto de interfaz/CAM.
No cambia los perfiles base ni habilita TX de red.

## Validación física

Cinco segundos en ARM: **0 transferencias USB**. CRC OK de PHY al finalizar:
CCK473, OFDM78, HT45. PLE grupo0 vacío y cuota6 con0 páginas usadas en las
muestras. No demuestra ausencia continua de ocupación ni identifica subtipo.

Cambios efectivos observados, todos restaurados al valor inicial:

| Control | Antes | Durante |
|---|---|---|
| Histograma20738 | 807100c0 | 807100cc |
| VHT MU20758 | 8002 | a002 |
| TB20760 | c006 | e006 |
| CCK20764 | 1 | 3 |
| HT2076c | 2 | 100002 |
| VHT20770 | 2 | 100002 |
| HE20774 | 6 | 100006 |
| EHT20788 | 6 | 100006 |

La traza verifica primera/última lectura idéntica para los19 controles.
La inicialización faltante es real; completarla en la ventana no bastó para
recibir por USB. El bloqueo sigue sin aislarse.

## Pruebas y siguiente contraste

41 suites C++ y46 tests Python; compilación ARM/Intel. El test nuevo cubre
mapeo reservado/EHT, campos por tipo de página, restauración tras fallo en
cada una de17 escrituras, fallo del callback y guardas sin escrituras.
También pasó AddressSanitizer/UndefinedBehaviorSanitizer.
Hashes/trazas: [physts-stage50-observation.json](physts-stage50-observation.json).
Binario ejecutado conservado; stderr vacío y scheduler de red pausado.

Próximo contraste con fuente: contexto de interfaz/puerto y CAM que Linux
crea además de la inicialización global, y su relación con aceptación y
despacho de tramas. No se programará una identidad/contexto suponiendo que
la mera existencia de CRC OK prueba entrega RMAC. Progreso ponderado40%.
