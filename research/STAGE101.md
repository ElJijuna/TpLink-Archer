# Etapa101 — sondeo tag8 con CCK PHY y captura externa simultánea

2026-11-03. El usuario ejecutó el coordinador --phy-drained en Terminal.

El drenaje corregido alcanzó quiet=true tras401 transfers y permitió los
nueve comandos de contexto/limpieza con ocho ACK. USB aceptó una sola trama
de104 bytes por endpoint5. El firmware reportó RETRY_LIMIT/tag8/count1.

La medición emparejada PHY, tomada antes del envío y después de pausar TX,
antes del tratamiento del terminal fallido y sin reset intermedio del host:

| Contador | Antes | Después | Delta módulo65536 |
|---|---:|---:|---:|
| CCK_TXEN |0|1|1|
| CCK_TXON |0|1|1|
| OFDM_TXEN |0|0|0|
| OFDM_TXON |0|0|0|

Controles habilitados. IFS-CLM: baseline0,ventana activa17 unidades32us
(544us internos); ventanas2003/2002ms. Los contadores confirman actividad
interna CCK asociada a la ventana del envío; no miden potencia ni prueban
emisión reconocible por antena. Los cuatro contadores/deltas son evidencia
más precisa que el IFS interno solo.

La captura independiente terminó con capture_exit0. CRC32 software sobre2818
registros:1979 válidos,835 inválidos,4 truncados,0 discrepancias con bad-fcs.
Todos los metadatos de canal son2452MHz. Hay395 beacons válidos de Especial,
149 ACK,107 CTS y0 tramas válidas con las direcciones de nuestra MAC.
No se demuestra ausencia de energía RF ni se atribuyen controles ajenos al
Archer. La restauración del canal36 y alternativa20MHz fallaron−3903;
monitor_exit1 no significa que tcpdump fallara.

La retirada del contexto terminó confirmada:776 transfers,310 paquetes
normales,259 beacons,TX pausado,bulk_async_drained pending0,contexto CLEAN.
El marcador tag8 se conserva consumido. Ledger77 reservadas/179 libres.
El texto genérico «reservation may remain DIRTY» de stderr no describe el
estado final observado: hubo limpieza reconocida antes de propagar el error
terminal de transmisión. No se reintenta tag8.

Objetivos:1) ejecutado y auditado el sondeo con PHY antes/después y captura
externa simultánea;2) fallo RF sin localizar/corregir todavía, DPK/TSSI no
plenamente interpretados;3)sin respuesta dirigida del AP,asociación,seguridad
ni tráfico de datos. Avance52%; no hay conexión Wi-Fi utilizable.

Siguiente contraste: concesiones Wi-Fi/BT y controles coexistentes en modo
pausado contra rtw89_mac_cfg_gnt_v2 y las definiciones RTL8922A. La política
BTC de producción aún no está implementada; eso plantea una hipótesis, no
prueba que explique el fallo. No se han forzado concesiones ni registros RF.

Evidencia íntegra con hashes:
[phy-probe-stage101-observation.json](phy-probe-stage101-observation.json).


## Diferencia física en las concesiones Wi-Fi/BT

Dos snapshots nuevos coinciden con los210 valores de ruta/potencia y pasan
validate_trace sin mutaciones fuera de WLAN/XTAL de lectura. Ambos leen
R_BE_GNT_SW_CTRL0xe348=0x01775774. Según rtw89-reg.h:6472–6498 y
rtw89_mac_cfg_gnt_v2 en rtw89-mac_be.c:2355–2405, WL_BB0/WL_TX/WL_RX
están bajo control software con valor0; BT_BB0 y BT_RX_BB0 están forzados
altos. WL_BB_PWR conserva control hardware. Es evidencia de una política
contraria a conceder Wi-Fi, no prueba todavía de causalidad del fallo RF.

Se implementó with_scc_wlan_grant_window para un contraste PHY0/SCC/MCC0:
máscara0x0003fc3f,valor bajo máscara0x00017c1f. Fuerza WL BB0/TX/RX/power
altos y BT BB0/TX/RX bajos, conservando grants BB1,WL_ACT y bits ajenos.
Rechaza cualquier política inicial distinta de la medida y requiere TX
pausado antes de aplicar/restaurar. No implementa coexistencia de producción.

Contraste físico sin H2C ni tramas del host: lectura original0x01775774,
escritura/readback0x01757f5f,restauración/readback0x01775774. Conservó los
210 valores del plan y179 secuencias libres, sin reservar ninguna.
El ledger y contexto siguen idénticos a la limpieza tag8. Es una ventana
validada de configuración, todavía no una corrección RF validada por antena.

107 tests Python y suites C++ aprobados. Las pruebas de la ventana comprueban
cada bit WL/BT,preservación de bits externos,restauración al fallar el cuerpo
y rechazo de scheduler/MCC/política inesperados. Binario tag9 universal
Intel/ARM compilado; Intel físico sigue pendiente.

El intento nuevo tag9 mantiene104 bytes/CCK1/noRTS salvo tag y secuencia.
Mide ambas ventanas IFS y PHY dentro de la concesión Wi-Fi y restaura la
política original antes de evaluar el terminal. Marcador propio; admisión
ligada al tag8 y contraste reversible mediante hashes,ledger77/179 yCLEAN.
PREPARADO,NO EJECUTADO al redactar este apartado.

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v
python3 tools/external_probe_capture.py --phy-grant artifacts/grant-probe-stage101
```

DPK/TSSI conservan sus incertidumbres anteriores. No se los declara causa
ni éxito global por haber encontrado la política de concesión. Asociación,
seguridad y datos siguen pendientes de comprobar respuesta del AP.
