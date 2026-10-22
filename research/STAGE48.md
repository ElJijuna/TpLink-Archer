# Etapa48 — contraste de filtros RX

Fecha: 2026-10-21. Cuatro ventanas físicas ARM de cinco segundos, sin
retune ni envío host de tramas o comandos firmware, con scheduler TX de red
pausado. Cada ventana restaura los registros y descarta previamente las
estadísticas PMAC mediante el reset diagnóstico ya validado.

| Prueba | Variación respecto al perfil base | Transferencias USB |
|---|---|---|
| 48a | RX OPT A_BC bit2:1→0 | 0 |
| 48b | RX OPT SNIFFER_MODE bit0:0→1 | 0 |
| 48c | Igual48b, añade lecturas BB | 0 |
| 48d | CMAC RESP_PKTCTL_EN bit7 habilitado | 0 |

La fuente fijada `c03c1a14507dfaf8b856c2608f9ac216021fc222` declara los
bits BE en `reg.h:8194-8215`. `fw.c:10424-10426` limpia los bits AX A_BCN,
A_BC y A1 en su ciclo de escaneo; aquí se contrasta solo A_BC en el registro
BE y no se afirma que sea el ciclo completo de escaneo. SNIFFER_MODE es una
hipótesis diagnóstica: no se presupone que respete las restricciones por subtipo.
Se mantienen los filtros CTRL/DATA en0 y MGNT en120; no llegaron payloads.

La prueba48d conserva el motor de respuestas como en el power-on de
`rtw8922a.c:622-626`. Puede permitir respuestas automáticas de hardware;
scheduler pausado y ausencia de escrituras bulk no prueban silencio RF absoluto.
No hubo tramas enviadas por el ejecutor ni comandos de asociación/probe.
No cambia el perfil base, que sigue desactivando ese motor temporalmente.

## Controles BB observados

48c añadió lecturas en PHY0, apertura BB+20000:

| Registro | Valor |
|---|---|
| R_RSTB_ASYNC 20704 | 601e0d02 |
| R_PMAC_GNT 20980 | 00005250 |
| R_PMAC_RX_CFG1 20988 | 3c3c4100 |
| R_PMAC_RXMOD 20994 | 00000000 |
| R_MAC_SEL 209a4 | 00000000 |

Según las máscaras declaradas, reset asíncrono bit1 liberado, PMAC TX/RX
grant bits0/16 desactivados, selección MAC bits19:17 en0. No se infiere el
significado de los demás bits. No se escribieron estos controles BB.

Todas las ventanas siguieron registrando CRC OK en PHY y0 transferencias USB.
PLE grupo0 estaba vacío y cuota6 marcaba0 páginas usadas en las muestras.
No se prueba que jamás entraran paquetes a PLE ni que estos bits carezcan de
efecto en otras condiciones. Las variaciones no bastaron para liberar RX.

## Validación y pendiente

39 suites C++ y46 tests Python, compilación universal ARM/Intel. Las pruebas
de ventana verifican los valores aislados de cada variante y su restauración.
No se elevó el avance40%. Próximo foco: camino de descriptor/PHY-status hacia
RMAC, filtros de UID y configuración de recepción que aún requiera contexto
firmware/CAM; contrastar cada hipótesis contra la fuente antes de escribir.

Trazas y hashes: [rx-filter-stage48-observation.json](rx-filter-stage48-observation.json).
Binario exacto48d conservado. Los binarios anteriores a48d no se conservaron.
Los cuatro stderr están vacíos y las cuatro ventanas terminaron restauradas.
