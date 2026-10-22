# Etapa51 — puerto sin enlace y protocolo de rol

Fecha: 2026-10-21. Fuente Linux fijada a
`c03c1a14507dfaf8b856c2608f9ac216021fc222`.

## Secuencia de interfaz identificada

`mac80211.c:169-235` añade interfaz, obtiene MACID/puerto y llama
`__rtw89_ops_add_iface_link`→`rtw89_mac_add_vif`→`rtw89_mac_vif_init`.
`mac.c:5045-5090` realiza, en orden:

1. Actualizar puerto.
2. Inicializar tablas DMAC/CMAC y quitar pausa MACID.
3. ROLE_MAINTAIN CREATE.
4. JOIN_INFO con dis_conn=true para el contexto inicial.
5. Inicialización CAM y envío CAM CREATE.
6. Tablas CMAC/DMAC predeterminadas del chip.

Esto no equivale a asociación Wi-Fi. La implementación actual todavía no
reconstruye esa transacción completa ni sus confirmaciones/limpieza firmware.

## Prueba física del puerto

Primera guarda conservadora rechazó la prueba antes de escribir el puerto:
PORT_CFG_P0=0001e01b, FUNC_EN=0, NET_TYPE=0 y BCN_TX/FORCE_TX=0,
pero BSSID_FIT/TSF_UPDATE estaban activos. WLAN IO se restauró al salir.

La fuente `mac.c:4659-4693` limpia BSSID_FIT y TSF_UPDATE en NO_LINK y
`mac.c:4929-4941` habilita PORT_FUNC. La prueba revisada acepta únicamente
puerto inactivo, NET_TYPE0 y transmisión beacon desactivada; aplica máscara1c,
valor4, y restaura esos campos al terminar. No escribe identidad, CAM o rol.
No implementa todos los temporizadores, reportes o parámetros de port_update.

Ventana física ARM de cinco segundos: PORT_CFG0001e01b→0001e007→0001e01b,
**0 transferencias USB**, scheduler de red0. Filtros/motor de respuestas
restaurados, stderr vacío. Se restauran campos de configuración; no se afirma
que contadores temporales TSF u otras consecuencias del tiempo sean reversibles.
Esta configuración parcial del puerto tampoco bastó para entregar RX.

## Codificador de rol offline

`interface_role.hpp` reconstruye ROLE_MAINTAIN estación CLIENT/MAC0 para
CREATE/REMOVE, MACID0..255 y puerto0..4, sin transporte ni ejecución USB.
Fuente `fw.c:4949-5006`, `fw.h:1809-1819,4691,4759-4761` y enums de `core.h`:
payload4 bytes, MACID[7:0], CLIENT0[9:8], modo[12:10], STATION1[16:13],
MAC0[18:17], puerto[21:19]. Reservados y MACID_EXT quedan0.
H2C categoría1/clase8/función4, recv_ack0/done_ack1 según la llamada fuente.
Los tests comparan bytes completos de CREATE/REMOVE con secuencia explícita.
No se consumieron secuencias del ledger ni se enviaron estos comandos.
Compatibilidad con el firmware residente y aceptación runtime aún sin probar.

## Validación y pendiente

43 suites C++ y46 tests Python, ejecutor universal ARM/Intel. Pruebas de
puerto cubren bits no relacionados, baseline con ajuste BSSID/TSF, fallo de
escritura/callback y rechazo de puerto activo, enlazado o con beacon TX.
Pruebas del codec cubren tramas completas, límites y modo inválido.
Hashes/trazas: [interface-stage51-observation.json](interface-stage51-observation.json).

Siguiente trabajo concreto: JOIN_INFO_BE y CAM/tablas de un contexto estación
sin enlace, confirmación de comandos y retirada del rol. La ausencia de rol
es una diferencia real con Linux, todavía no una causa demostrada de RX0.
Avance ponderado40%.
