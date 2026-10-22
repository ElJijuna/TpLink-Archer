# Etapa52 — JOIN_INFO BE y CAM V0 offline

Fecha: 2026-10-21. Codificadores derivados de la revisión Linux fijada
`c03c1a14507dfaf8b856c2608f9ac216021fc222`. No se ejecutaron comandos USB,
no se consumieron secuencias del ledger y no se crearon roles residentes.

## JOIN_INFO

`encode_unlinked_station_join` reconstruye el payload BEv1 de12 bytes para
un único enlace estación designado, no-MLD, MAC0, NO_LINK, dis_conn=true.
MAIN_MACID=self; CLIENT self-role0, STATION Wi-Fi role1; sin trigger,
power-save inicial, EMLSR, NSTR o asociación. STA_TYPE admite los códigos
fuente legacy0/HE1/EHT2, sin inferir capacidades anunciadas por el dispositivo.

Fuente `fw.c:5045-5136`, `fw.h:1821-1863`. Las constantes se verificaron en
`include/linux/ieee80211-eht.h:505,513`: padding256us=4 y transición256us=5.
W1 incorpora MLSR1 y esas constantes incluso con IS_MLD0, igual que la fuente.
H2C categoría1/clase8/función0, recv_ack0/done_ack1.

Las pruebas comparan el frame completo de20 bytes y los campos límite.
La correlación usa el `CommandTracker` existente: ACK sintético de otro
comando o secuencia no completa JOIN; error de firmware conserva su código;
duplicado idéntico se admite y completitud contradictoria se rechaza.
Esto verifica el parser offline, no recepción de ACK real para estos comandos.

## CAM: dos versiones distintas

`rtw8922a.c:3289` declara addrcam_ver0 para los campos de entrada. Además,
`fw.c:932` selecciona la característica **ADDR_CAM_V0 para firmware menor
que0.35.84.0**, incluida la imagen cargada0.35.41.0.
Esa segunda decisión selecciona el payload60 bytes, sin el W15 añadido en
el formato64 bytes. No se confunden versión de campos y longitud del comando.

`encode_unlinked_cam_v0` construye ese payload para PHY0/NO_LINK, BSSID cero,
dirección local unicast explícita, sin claves, WoW, AID o trigger. Valida
índiceADDR<128, índiceBSSID<20, MACID<=255 y puerto<=4 según el chip.
Rechaza dirección cero/multicast como restricción conservadora del encoder.

Fuente `cam.c:646-929`, `cam.h:15-116`, `mac.h:15-17`: entradaADDR corta32,
BSSID8, hashSMA XOR de seis bytes, TMA/BSSID cero, máscara BSSID completa63,
seguridad NORMAL2 sin entradas válidas. W0/W7 y reservados quedan cero,
conforme a la puesta a cero del allocator `fw.c:91`.
CREATE marca ambas entradas válidas; REMOVE invalida ambas conservando
los demás campos, como deinit+CAM de la fuente. En V0 no hay UPD_MODE W15.
H2C categoría1/clase6/función0, recv_ack0/done_ack1; frame completo68 bytes.

La dirección de las pruebas es sintética y no se programó en el adaptador.
La selección V0 se justifica para la versión indicada; el encoder no debe
usarse con otro firmware sin resolver de nuevo la característica de formato.

## Validación y límites

45 suites C++ y46 tests Python. CAM pasó AddressSanitizer/UBSan;
el test de JOIN compila universal ARM/Intel. Goldens completos para JOIN y
CAM CREATE/invalidación, guardas de campos y correlación ACK negativa.
Hashes de fuentes y registro de pruebas:
[interface-protocol-stage52.json](interface-protocol-stage52.json).

Pendientes antes de ejecutar una interfaz completa: inicialización/tablas
DMAC/CMAC del chip, política de pausa MACID manteniendo scheduler global
detenido, propiedad de índices CAM/rol, confirmaciones acotadas y limpieza
tras resultado incierto. Ni un payload codificado ni un ACK de aceptación
demuestran que la ruta RX funcione. Último resultado físico sigue siendo
RX USB0 en etapa51b. Progreso ponderado40%.
