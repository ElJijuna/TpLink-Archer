# Archer USB — investigación y base de controlador

## Checklist resumido — avance estimado: **60 %**

Actualizado: **2026-11-06**, etapa124: auditoría offline ratifica seis tramas CCMP de grupo, incluido DISCOVER propio reflejado; cero OFFER. Cinco solicitudes ARP autenticadas, sin relación DHCP demostrada. Último contexto registrado DIRTY,206 secuencias libres. Objetivo: controlador Wi-Fi funcional
para macOS ARM/Intel, con Linux posteriormente.

- [x] Identificación del dispositivo y protocolo USB/registros — **10/10**.
- [x] Carga experimental de firmware principal y BBMCU0 — **10/10**.
- [x] DLE/HFC/DMAC/CMAC0 y tablas BB/radio/NCTL en USB2/SCC sobre Mac ARM — **15/15**.
- [ ] Calibraciones y validación RF completas — **5/10**: comandos ejecutados; faltan semánticas DPK/TSSI y contraste de calidad RF.
- [ ] Ruta completa de datos Wi-Fi TX/RX — **11/15**: gestión corroborada externamente y tráfico cifrado de ida/vuelta de grupo autenticado en captura nativa; faltan recepción unicast, DHCP/IP y el resto del camino.
- [ ] Escaneo, asociación y seguridad Wi-Fi — **9/15**: barrido pasivo1/6/11, asociación aceptada, M3 autenticado, M2/M4 reproducidos y CCMP de grupo validado; faltan ciclo de claves completo y ampliación.
- [ ] Integración macOS: DriverKit, interfaz de red, instalación y ciclo de vida — **0/15**.
- [ ] Validación física en Mac Intel — **0/5**.
- [ ] Implementación y validación en Linux — **0/5**.

**60/100 puntos de hitos ponderados.** Es una estimación de alcance completado,
no del tiempo restante. Compilar para Intel no equivale a validarlo físicamente.
El adaptador todavía no ofrece conexión Wi-Fi utilizable.

**Estado actual:** CMAC0 completo para el perfil USB2/SCC, DLE/HFC/DMAC,
tablas de radio y secuencia TXGAPK/IQK/TSSI/DPK ejecutados en el Mac ARM.
La repetición desde firmware recargado terminó con respuestas OK de todos
los comandos y RXDCK final limpio. IQK y TXGAPK reportan éxito; DPK no muestra
timeouts/overflow, pero su máscara de estado y los estados de alineación TSSI
siguen sin interpretación definitiva. No se afirma calidad RF plenamente validada.
El último postflight dejó TX de red pausado. Especial aceptó autenticación y
asociación (AID1 en116); M3 fue autenticado y M2/M4 reproducidos offline.
El AP reenvió por CCMP de grupo una copia exacta del DISCOVER cifrado enviado;
también se autenticaron tres tramas ARP. Faltan OFFER, lease, recepción
unicast cifrada e interfaz de red.
Los ejecutables compilan para Intel/ARM; falta prueba física en Intel y Linux.
Evidencia y límites: [research/STAGE42.md](research/STAGE42.md).

Etapa124: auditoría real completada,260 hashes de entrada verificados.
M2/M4 reproducidos, M3 autenticado, DISCOVER reflejado y cinco solicitudes ARP
verificados. Cero OFFER/pairwise; los metadatos ARP no permiten atribuir causa
al fallo DHCP. Sin USB, TX nuevo ni liberación de contexto.
[Resultados y límites](research/STAGE124.md).

Etapa123: intento122 ejecutado; M3 autenticado, DISCOVER broadcast reflejado
y cinco ARP autenticados nativamente, cero rechazos/OFFER/pairwise. El usuario
confirma DHCP operativo en otro cliente. Postflight210 registros correcto;
contexto DIRTY y206 secuencias libres. Auditor ARP offline preparado y123
pruebas Python más suites C++ pasan. No repetir122.
[Resultado y auditoría sin radio](research/STAGE123.md).

Etapas121–122: recuperación fría verificada, nueva época, contexto CLEAN,
215 secuencias libres y210 registros coincidentes. Preparado un intento nuevo
que solicita OFFER broadcast, con observación CCMP por tipo de recepción;
123 pruebas Python y suites C++ pasan. Sin nuevo TX Wi-Fi ni OFFER todavía.
[Recuperación121](research/STAGE121.md), [contraste122](research/STAGE122.md).

Etapa120: tras reconectar, Archer aparece como instalador Realtek0bda:1a2b.
La expulsión requiere autenticación sudo local; sin recarga, H2C o TX nuevos.
[Continuación de recuperación](research/STAGE120.md).

Etapa119: receptor instrumentado para distinguir CCMP autenticado de grupo o
unicast, DISCOVER reflejado y OFFER válido, también al finalizar por timeout.
Suites C++ y122 pruebas Python pasan; compilación ARM/Intel. Inspección actual
`cold_profile=false`: sin TX, H2C ni reset nuevos; recuperación fría pendiente.
[Instrumentación y siguiente paso](research/STAGE119.md).

Etapa118: auditoría real completada sin USB ni TX nuevo. Los253 hashes de
entrada y el ejecutable preparado coinciden. Cuatro tramas CCMP autenticadas,
cero rechazadas: DISCOVER propio reflejado y tres ARP; cero OFFER. Evidencia
de ida/vuelta cifrada de grupo, con contexto DIRTY conservado.
[Resultados y límites](research/STAGE118.md).

Historial reciente (los pendientes describen el estado de cada etapa):

Etapa117: el intento116 ya se ejecutó. M3 verificado y M4/DISCOVER enviados;
la espera falló al no confirmar OFFER. Cuatro tramas cifradas del AP permiten
seguir mediante auditoría offline, sin otro TX. Error de directorio existente
aclarado;122 pruebas Python y suites C++ aprobadas.
[Resultados y comando de auditoría](research/STAGE117.md).

Etapas115–116: época nueva y contexto pasivo verificados;215 secuencias
libres, TX pausado y210 registros correctos. Nuevo intento con SW_MLD de datos
corregido y contadores PHY por fase preparado;120 pruebas Python y suites C++
aprobadas. El efecto sobre M2/M3 sigue pendiente por radio.
[Recuperación115](research/STAGE115.md), [instrucciones116](research/STAGE116.md).

Etapa114: lectura física confirma37ad:0104 con `cold_profile=false`;
no se inició reset, H2C ni nuevo intento. Ledger206 libres y contexto DIRTY
conservados. [Estado y evidencia](research/STAGE114.md).

Etapa113: Especial aceptó asociación/AID1 y envió cuatro M1. M2 se entregó
por USB09, pero no llegó M3 ni se encontró M2 en captura externa. Se corrigió
un SW_MLD heredado indebidamente en datos, con regresión que falla antes y
pasa después;120 pruebas Python y suites C++ aprobadas. Causalidad por radio
aún pendiente. TX pausado, concesión restaurada, contexto DIRTY,206 libres.
[Evidencia y recuperación necesaria](research/STAGE113.md).

Etapas111–112: nueva época fría identificada, bootstrap y canal9 restaurados;
contexto creado/retirado con ocho ACK,408 paquetes normales y TX pausado.
Postflight210 registros correcto. Nuevo intento WPA2 con secuencias22–27
preparado;120 pruebas Python y suites C++ aprobadas. Sin WPA2/datos nuevos
todavía. [Recuperación111](research/STAGE111.md),
[instrucciones y límites112](research/STAGE112.md).

Etapas109–110: recarga fría completa, radio/canal9 y contexto pasivo verificados;
215 secuencias libres al cerrar109. La enumeración actual volvió a0bda:1a2b,
por lo que ese estado histórico no se trata como firmware vivo. Nuevo intento
WPA2 compilado pero bloqueado;120 pruebas Python y suites C++ aprobadas.
[Recuperación109](research/STAGE109.md), [estado e instrucciones110](research/STAGE110.md).

Etapa108: modo instalador0bda:1a2b y CD Realtek verificado. La interfaz USB
está ocupada; expulsión óptica pendiente de autenticación en Terminal. Sin
SCSI/H2C/tramas nuevas;118 pruebas Python y suites C++ aprobadas.
[Instrucciones y evidencia](research/STAGE108.md).

Etapa107: enumeración37ad:0104; el perfil frío no aparece. La recuperación
se detuvo antes de reset/carga/H2C y conservó el ledger. Continuación segura
preparada y118 pruebas Python más suites C++ aprobadas; pendiente desconexión
eléctrica e inspección fría. [Evidencia](research/STAGE107.md).

Etapa106: el primer H2C (secuencia104) llegó por USB sin receive ACK.
No hubo autenticación, EAPOL ni DHCP nuevos y la contraseña no fue puesta a prueba.
Contexto DIRTY,113 secuencias consumidas/143 libres; intento y reservas conservados.
No volver a ejecutar el comando105. [Evidencia y límites](research/STAGE106.md).

Etapas102–104: la ventana reversible de concesiones Wi-Fi permitió obtener
solicitud y respuesta de Especial en captura independiente (tag9). Después,
autenticación y asociación fueron aceptadas y corroboradas externamente
(tags10–12). Con tags13–15 se recibió M1 WPA2 nativo con FCS válido;
la captura externa no corroboró M1. Concesiones/filtros restaurados, contexto
CLEAN y152 secuencias disponibles. [Etapa102](research/STAGE102.md),
[etapa103](research/STAGE103.md), [etapa104](research/STAGE104.md).

Etapa105: intercambio WPA2, CCMP software y DHCP DISCOVER cifrado preparados.
Suites C++ y116 pruebas Python aprobadas; ejecutable ARM/Intel compilado.
Postflight físico sin TX/H2C correcto. Falta ejecutar con contraseña introducida
localmente en Terminal; todavía no se afirma seguridad ni datos cifrados.
[Instrucciones, alcance y límites](research/STAGE105.md).

Historial de etapas anteriores (sus estados pendientes corresponden a esa fecha):

Etapa101: tag8 ejecutado con captura independiente; CCK_TXEN/TXON0→1,
OFDM0→0,pero RETRY_LIMIT/count1. CRC externo1979 válidos,835 inválidos,
4 truncados;395 beacons de Especial y0 tramas válidas de nuestra MAC.
Limpieza confirmada,CLEAN,179 secuencias libres,TX pausado. Primer objetivo
medido; fallo RF,DPK/TSSI y asociación siguen abiertos. Avance52 %.
Se observó además una política que fuerza bajas las concesiones Wi-Fi;
ventana correctiva reversible validada sin TX,tag9 preparado para contrastar
causalidad por radio. No se declara aún corregido el fallo RF.
Evidencia: [research/STAGE101.md](research/STAGE101.md).

Etapa100: captura externa del intento tag7 ejecutada; el ejecutor se detuvo
antes de H2C/TX porque no drenó la entrada. CRC externo4069 válidos/727
inválidos/5 truncados,395 beacons de Especial. Marcador tag7 conservado,
188 secuencias libres. Drenaje corregido y probado físicamente sin H2C/TX;
tag8 preparado con marcador nuevo, pendiente de ejecución en Terminal.
No hay medición PHY con TX nueva, corrección RF ni asociación. Avance52 %.
Evidencia: [research/STAGE100.md](research/STAGE100.md).

Etapa99: sondeo único tag7/secuencia7 con captura externa y contadores PHY
antes/después preparado. Preflight9/TX pausado y105 tests Python aprobados;
ejecución pendiente de autenticación sudo en Terminal. Sin nuevo TX ni
asociación. Contexto CLEAN y197 secuencias, avance52 %.
Evidencia: [research/STAGE99.md](research/STAGE99.md).

Etapa98: tres snapshots de210 registros estables, estado persistente intacto
y contraste separado de ruta AB/tasas CCK. CRC software externo:1463 válidos,
1010 inválidos,4 truncados;0 tramas válidas de nuestra MAC. Instrumentación
PHY antes/después integrada sin nuevo TX.103 tests Python y suites C++ pasan.
Se corrigió la explicación previa del reset de contadores para RETRY_LIMIT.
Contexto CLEAN,197 secuencias, avance52 %.
Evidencia: [research/STAGE98.md](research/STAGE98.md).

Etapa97: auditor físico de ruta/potencia compilado Intel/ARM y ejecutado en
Mac ARM. Los210 registros coinciden con el plan; continuidad TSSI bit3 limpia
en ambos caminos. No valida calidad RF ni suficiencia del plan. Sin nuevo TX,
contexto CLEAN y197 secuencias. Avance52 %.
Evidencia: [research/STAGE97.md](research/STAGE97.md).

Etapa96: contraste simultáneo completado: tag6 RETRY_LIMIT/count1; captura
externa2477 paquetes en9 y381 beacons de Especial sin marca bad-fcs, pero
ninguna trama identificada con nuestra MAC. No demuestra ausencia de energía
RF. Contexto CLEAN,197 secuencias y TX pausado; Wi-Fi del Mac requiere
reconexión manual tras fallar restauración. Avance52 %.
Evidencia: [research/STAGE96.md](research/STAGE96.md).

Etapa95: prueba simultánea preparada con captura independiente y sondeo
único tag6; exige observar Especial a2452MHz antes de autorizar el envío.
Compilaciones y94 tests Python pasan; todavía sin ejecutar TX nuevo.
Avance52 %. Evidencia: [research/STAGE95.md](research/STAGE95.md).

Etapa94: captura independiente de1499 paquetes, todos a2452MHz;165 beacons
de Especial sin marca bad-fcs,16 ACK y72 CTS del entorno. Sin nuevo TX Archer.
La captura terminó; restaurar el canal del Mac falló y requiere reconexión.
Avance52 %. Evidencia: [research/STAGE94.md](research/STAGE94.md).

Etapa93: receptor independiente en0 confirmado con radiotap y soporte9/20MHz.
Helper de captura pasiva20s compilado Intel/ARM; ejecución administrativa y
validación del canal en la captura pendientes. Sin nueva trama Archer.
Avance52 %. Evidencia: [research/STAGE93.md](research/STAGE93.md).

Etapas91–92: firmware recuperado y ocho terminales de calibración en9.
La captura pasiva recibió2 ACK y1 CTS con FCS válido. Un único sondeo nuevo
con captura de controles habilitada terminó en RETRY_LIMIT/tag5/count1,
sin ACK ni respuesta dirigida a nuestra MAC. Contexto CLEAN,206 secuencias,
TX pausado. Sigue pendiente comprobar la emisión con un receptor independiente.
Avance52 %. Evidencia: [research/STAGE91.md](research/STAGE91.md), [research/STAGE92.md](research/STAGE92.md).

Etapa90: auditor offline DPK/TSSI con campos completos; el patrón path4 y
códigos TSSI0 aparece tanto en1 como en9. Windows etiqueta el estado de
alineación0 como Disable, pero eso no valida éxito/fallo global del firmware
usado. Sin nuevo USB/TX ni cambios de calidad declarada. Avance52 %.
Evidencia: [research/STAGE90.md](research/STAGE90.md).

Etapas88–89: monitor IFS-CLM validado con ventanas pasivas de2s. Nuevo
sondeo unicast con RTS_EN desactivado: baseline TX0, ventana activa17 unidades
(544us internos), pero RETRY_LIMIT/tag4/count1 y0 respuestas a nuestra MAC.
Controles restaurados, contexto CLEAN,197 secuencias y TX pausado. Avance52 %.
Evidencia: [research/STAGE88.md](research/STAGE88.md), [research/STAGE89.md](research/STAGE89.md).

Etapa87: limpieza86 reconstruida desde USB y estado físico9/MCC0/TX pausado
verificado. Reserva recuperada a CLEAN sin reset, H2C ni trama nueva; siguen
206 secuencias. Contadores crudos no prueban emisión RF. Avance52 %.
Evidencia: [research/STAGE87.md](research/STAGE87.md).

Etapa86: Especial anuncia las cuatro tasas básicas CCK incluidas; esa
hipótesis no explica la ausencia de respuesta. Un probe unicast nuevo al AP
reportó RETRY_LIMIT/tag3/count1. Limpieza6/7/8 confirmada, TX pausado y USB
drenado; el fallo conserva reserva DIRTY,206 secuencias. Sin éxito externo.
Avance52 %. Evidencia: [research/STAGE86.md](research/STAGE86.md).

Etapa85: una solicitud de sondeo SSID Especial enviada en9 (104 bytes,tag2),
TX_DONE/count1 y limpieza confirmada. Captura:31 beacons y13 probe responses
 del objetivo, pero0 respuestas a nuestra MAC; aún sin evidencia externa TX.
Contexto CLEAN,215 secuencias y TX pausado. Avance52 %.
Evidencia: [research/STAGE85.md](research/STAGE85.md).

Etapa84: TXGAPK/IQK/TSSI/DPK/RXDCK ejecutados en9 con ocho terminales OK.
IQK coincide con9 y RXDCK replay confirma ambos caminos9 sin timeouts.
Estado final9/2452MHz, MCC0, contexto CLEAN,224 secuencias y TX pausado.
Calidad/semánticas DPK y TSSI aún pendientes; sin nueva trama de red.
Avance52 %. Evidencia: [research/STAGE84.md](research/STAGE84.md).

Etapa83: secuencia de calibración9 integrada y compilada Intel/ARM; tests
TSSI verifican sus grupos y campos DE. La ejecución se detuvo antes de H2C:
endpoint0x84 bloqueado y lectura de identidad abortada. Pendiente reconexión;
no se completaron calibraciones físicas en9. Avance52 %.
Evidencia: [research/STAGE83.md](research/STAGE83.md).

Etapa82: generador de potencia separado para canal9, con197 escrituras y
consulta del índice8 confirmado en Linux. Canal1 conserva exactamente su
salida previa;85 tests Python pasan. Sin nuevo TX ni calibración física.
Avance52 %. Evidencia: [research/STAGE82.md](research/STAGE82.md).

Etapa81: el Archer identificó Especial en9:26 beacons y2 probe responses
observados pasivamente. Recorrido1→9→1 con RXDCK limpio, FCS0 y replays
idénticos. Contexto recuperado/CLEAN, TX pausado y228 secuencias disponibles.
Sin nueva trama de red TX. Avance52 %.
Evidencia: [research/STAGE81.md](research/STAGE81.md).

Etapa80: auditoría reproducible con FCS confirma0 respuestas a la MAC de prueba.
No se detectaron tasas básicas omitidas en beacons de16 BSSID que anuncian1.
Objetivo Especial: resultado CoreWLAN con nombre oculto en9; retune pasivo9
preparado sin prueba física. Nuevo comando MAC sin ACK dejó contexto DIRTY;
TX pausado verificado y recuperación eléctrica pendiente. Sin nuevo TX Archer.
Avance52 %. Evidencia: [research/STAGE80.md](research/STAGE80.md).

Etapa79: solicitud de sondeo corregida entregada una vez por USB; firmware
reportó TX_DONE/count1. Contexto retirado, CLEAN y TX pausado. No se capturaron
probe responses dirigidos a la MAC de prueba; sin ACK externo ni asociación.
Avance52 %. Evidencia: [research/STAGE79.md](research/STAGE79.md).

Etapa78: tras reconexión eléctrica, carga fría, RFK y barrido1/6/11 completados.
34 BSSID distintos, tres replays idénticos, retorno a canal1 y contexto CLEAN.
Quedan223 secuencias; TX de red pausado y prueba de trama aún bloqueada.
Reset en caliente sin causa resuelta. Avance50 %.
Evidencia: [research/STAGE78.md](research/STAGE78.md).

Etapa77: recepción RFK asíncrona y plazo USB corregido probados tras recargas
verificadas; PRE_NOTIFY sigue sin terminal. El firmware sí responde capacidades
PHY por buzón. TX pausado; pendiente contraste con reconexión eléctrica.
Avance50 %. Evidencia: [research/STAGE77.md](research/STAGE77.md).

Etapa76: reset MAC/DLE y recarga verificados recuperaron el contexto DIRTY.
La restauración posterior se detuvo por falta de terminal PRE_NOTIFY; no hay
nuevo barrido ni nueva trama TX. Scheduler pausado verificado y prueba TX
bloqueada. Se corrigió la clasificación de reportes intermedios. Avance50 %.
Evidencia: [research/STAGE76.md](research/STAGE76.md).

Etapa75: primera solicitud de sondeo entregada por USB0x05, con reportes TX
reales del firmware. Estado: límite de reintentos, sin TX exitoso confirmado.
Scheduler TX pausado y verificado; contexto DIRTY pendiente de recuperación.
El ejecutable bloquea otro envío. Avance50 %.
Evidencia: [research/STAGE75.md](research/STAGE75.md).

Etapa74: plan USB TX con descriptor+trama, mapeo DMA/endpoint y tratamiento
de frontera512 mediante ZLP. Pruebas offline correctas; aún sin envío ni
confirmación TX por radio. Avance50 %.
Evidencia: [research/STAGE74.md](research/STAGE74.md).

Etapa73: extensión WDINFO de32 bytes integrada al cuerpo TX, con campos de
reintentos y solicitud de reporte. Descriptor64 comprobado offline; no hay
envío USB ni confirmación TX por radio. Avance50 %.
Evidencia: [research/STAGE73.md](research/STAGE73.md).

Etapa72: codificador offline del cuerpo TX BE v2 de32 bytes, con validación
de campos y vector de prueba. No hay envío USB ni TX físico; faltan extensión,
colas y reportes de transmisión. Avance50 %.
Evidencia: [research/STAGE72.md](research/STAGE72.md).

Etapa71: flujo completo desde carga de firmware hasta barrido pasivo1/6/11,
probado en ARM:49 BSSID distintos, tres replays idénticos y retorno a canal1.
Secuencias216→188; contexto CLEAN y TX de red pausado. Avance50 %.
Evidencia: [research/STAGE71.md](research/STAGE71.md).

Etapa70: informe conjunto por BSSID/SSID con procedencia de recepción1/6/11.
Las capturas guardadas de etapa68c contienen45 BSSID distintos y coinciden
con los tres replays offline. No es un nuevo escaneo físico. Avance50 %.
Evidencia: [research/STAGE70.md](research/STAGE70.md).

Etapa69: pulso de tres segundos en GPIO8 ejecutado sobre ARM con readbacks
correctos y restauración de registros verificada. Herramienta universal y
pruebas de recuperación ante fallos listas. El usuario confirmó visualmente
el encendido en la repetición de15 segundos. Avance50 %; encender el LED
no completa la conexión Wi-Fi.
Evidencia: [research/STAGE69.md](research/STAGE69.md).

Etapa68: barrido pasivo1→6→11→1 probado dos veces sobre ARM, con RXDCK
limpio por destino y retorno comprobado a2412MHz. La versión final recibió
127/144/100 observaciones válidas en6/11/1; todos los agregados coinciden con
el replay offline y no tienen errores FCS. Quedan216 secuencias. TX de red
pausado; calidad RF completa aún no validada. Avance50 %.
Evidencia: [research/STAGE68.md](research/STAGE68.md).

Etapa67: planes BB/RF para1/6/11 y búsqueda MCC con historial explícito,
verificados contra la fuente y los datos enlazados de placa. El exportador es
offline y no ejecutable; falta integrar RFK, potencia, captura y restauración.
No se ha retuneado el adaptador. Escaneo operativo aún en1; avance48 %.
Evidencia: [research/STAGE67.md](research/STAGE67.md).

Etapa66: transición explícita de época probada con reset/recarga sin H2C,
archivo anterior preservado y bloqueo durable de fallos. La reanudación obtuvo
16 entradas y417 observaciones válidas, sin errores FCS. Tras consumir22
secuencias en la época nueva quedan232. No hay wrap dentro de una época.
Avance48 %. [research/STAGE66.md](research/STAGE66.md).

Etapa65: coordinador de recuperación, calibración y escaneo probado sobre ARM.
Conserva binarios/logs/manifiesto y bloquea otras herramientas actualizadas entre
fases. Obtuvo14 entradas y250 observaciones válidas, con replay idéntico.
Quedan11 secuencias; otro ciclo completo exige22 y se rechazará antes de tocar
USB. Falta una transición de época segura antes de más ciclos completos.
Avance48 %. [research/STAGE65.md](research/STAGE65.md).

Etapa64: escaneo pasivo agregado en17 entradas a partir de293 observaciones
válidas (238 beacons y55 probe responses), sin sondas activas. Verifica FCS,
conserva SSID como bytes y separa canal recibido de canal anunciado. Resultado
en vivo idéntico al replay offline; todavía sin barrido ni asociación. Avance48 %.
Evidencia: [research/STAGE64.md](research/STAGE64.md).

Etapa63: tras recuperar el arranque, dos ciclos recibieron555 paquetes normales,
incluidos448 beacons y77 probe responses. Los555 FCS coinciden con CRC32
calculado independientemente. Ocho controles PHY correctos en ambas pruebas;
contexto retirado y TX de red pausado. RX reproducido sobre ARM; falta integrar
el arranque, escaneo pasivo estructurado, TX y asociación. Avance45 %.
Evidencia: [research/STAGE63.md](research/STAGE63.md).

Etapa62: auditoría de24 campos RMAC/PLE/USB antes, durante y después
del contexto. No aparecen diferencias inesperadas frente a la fuente bajo
las máscaras medidas; las ventanas restauraron los24 valores iniciales.
PHY activa y ocho ACK, aún sin tramas normales. Sigue el contraste de la
interfaz PHY→MAC y aceptación de tramas. Avance40 %.
Evidencia: [research/STAGE62.md](research/STAGE62.md).

Etapa61: recepción asíncrona persistente implementada y probada sobre ARM:
ocho lecturas en 0x84 y una en 0x88, sin timeout USB. Recibió ocho ACK,
pero ninguna trama normal; las nueve peticiones pendientes se cancelaron
y drenaron antes de liberar buffers. PHY activa y contexto retirado.
El avance permanece en40 %; falta contrastar la entrega RMAC/PLE/USB.
Evidencia: [research/STAGE61.md](research/STAGE61.md).

Etapa60: telemetría de lecturas USB confirma ocho ACK en 0x84 y cero
tramas normales. Los 792 timeouts no aportan un tamaño válido según el
contrato de IOKit. PHY activa; siguiente contraste: recepción asíncrona
persistente como en la fuente Linux. El avance permanece en40 %.
Evidencia: [research/STAGE60.md](research/STAGE60.md).

Etapa43: el decodificador RX ya expone errores CRC/ICV, tasa, ancho de banda
y conversión de cabecera. Una captura anterior contiene un paquete normal
de 124 bytes sin errores de descriptor y con cabecera sin convertir. Esto
valida el análisis de esa captura; falta recepción reproducible con CMAC
actual, validación de tramas y escaneo pasivo. El avance permanece en **40 %**.
Detalles: [research/STAGE43.md](research/STAGE43.md).

Etapa44: parser offline de beacons/probe-responses con límites de cabecera e
información variable, probado con tramas sintéticas. FCS requiere política
explícita; todavía falta comprobar recepción pasiva con CMAC actual.
Detalles y uso: [research/STAGE44.md](research/STAGE44.md).

Etapa45: ventana RX probada físicamente durante cinco segundos en ARM:
filtros y motor de respuestas restaurados, TX pausado, **0 transferencias**.
Falta localizar el bloqueo del recorrido RX; avance **40 %**.
Evidencia: [research/STAGE45.md](research/STAGE45.md).

Etapa46: contadores PHY reiniciados y medidos físicamente: **CCK376,
OFDM31 y HT5 paquetes con CRC correcto** durante cinco segundos, pero
**0 transferencias USB**. La radio decodifica tráfico; falta aislar el bloqueo
posterior en RMAC/DLE/despacho/USB. Avance **40 %**.
Evidencia: [research/STAGE46.md](research/STAGE46.md).

Etapa47: PLE grupo0 vacío y cuota RX sin uso en las muestras. Activar
temporalmente RMAC CH_EN tampoco entregó paquetes USB; se restauró el bit.
Continúa el diagnóstico de filtros y entrega PHY→RMAC. Avance **40 %**.
Evidencia: [research/STAGE47.md](research/STAGE47.md).

Etapa48: contrastados A_BC, SNIFFER_MODE y motor de respuestas por separado;
ninguno liberó RX USB. Controles BB consultados no muestran PMAC TX/RX grant
activo ni reset asíncrono retenido. Se conserva evidencia y avance **40 %**.
Detalles: [research/STAGE48.md](research/STAGE48.md).

Etapa49: se contrastó PHY report de ocho bytes siguiendo la fuente; campos
aplicados y restaurados, todavía **0 transferencias USB**. Se identificó
inicialización PHY-status pendiente de portar. Avance **40 %**.
Detalles: [research/STAGE49.md](research/STAGE49.md).

Etapa50: inicialización PHY-status no-monitor portada y probada en ventana
reversible. Se restauraron19 controles; PHY contó CRC OK, pero USB continúa
en **0 transferencias**. Falta aislar el contexto/entrega RMAC. Avance **40 %**.
Detalles: [research/STAGE50.md](research/STAGE50.md).

Etapa51: puerto0 sin enlace contrastado y restaurado; todavía RX USB0.
Codificador CREATE/REMOVE de rol estación implementado y probado offline.
Falta la transacción completa de interfaz: JOIN, CAM, tablas y confirmaciones.
Avance **40 %**. Detalles: [research/STAGE51.md](research/STAGE51.md).

Etapa52: JOIN_INFO BE y CAM V0 de60 bytes implementados y probados offline,
con invalidación CAM y correlación de ACK. No se enviaron al adaptador;
faltan tablas DMAC/CMAC y ejecución completa con limpieza. Avance **40 %**.
Detalles: [research/STAGE52.md](research/STAGE52.md).

Etapa53: tablas por MACID CMAC G7/DMAC V2 y pausa/sueño individual
implementadas y probadas offline, con máscaras y políticas ACK de la fuente.
Falta coordinarlas y validarlas físicamente. Avance **40 %**.
Detalles: [research/STAGE53.md](research/STAGE53.md).

Etapa54: coordinador del contexto estación implementado y probado offline:
orden de nueve comandos, políticas ACK, retirada controlada y bloqueo ante
resultado incierto. Falta ejecutor USB y validación física. Avance **40 %**.
Detalles: [research/STAGE54.md](research/STAGE54.md).

Etapa55: contexto estación ejecutado y retirado físicamente dos veces:
**9 H2C y8 ACK por ejecución**, TX de red pausado y reserva final CLEAN.
USB de control funciona; todavía **0 tramas Wi-Fi normales**. Avance **40 %**.
Detalles y límites: [research/STAGE55.md](research/STAGE55.md).

Etapa56: agregación RXv3 de USB y filtro completo de gestión contrastados
con contexto reconocido; ocho ACK por prueba, todavía **0 paquetes normales**.
Configuraciones restauradas y contexto retirado. Avance **40 %**.
Detalles: [research/STAGE56.md](research/STAGE56.md).

Etapa57: se detectó PHY sin incremento y flags RMAC59hex durante el ciclo
de contexto. La secuencia conocida de firmware/radio/calibración recuperó
**349 CCK y49 OFDM con CRC correcto**, aún sin RX USB normal. Una retirada
reconocida no garantiza PHY activa; falta aislar qué transición la detiene.
Base recuperada, TX pausado y avance **40 %**.
Detalles: [research/STAGE57.md](research/STAGE57.md).

Etapa58: desde la base recuperada, PHY siguió recibiendo tras cada comando
y después de retirar el contexto. No se reprodujo la parada anterior ni se
aisló su causa. RX USB normal continúa en0. Avance **40 %**.
Detalles: [research/STAGE58.md](research/STAGE58.md).

Etapa59: observados flags PHYINFO0e, CMAC40 y dispatcher c0, presentes
desde antes del contexto, sin bits nuevos. PHY sigue contando CRC correctos,
pero RX USB normal continúa0. Preflight sin H2C corregido y reserva liberada
sin reutilizar secuencias. Avance **40 %**.
Detalles: [research/STAGE59.md](research/STAGE59.md).


Proyecto experimental para TP-Link USB `37ad:0104`, con destino macOS Intel/Apple Silicon y Linux posteriormente.

**Estado: diagnóstico y protocolo USB parcial. Todavía no es un controlador Wi-Fi funcional.** No instala extensiones. `--read-startup-state` consulta tres registros documentados (0xF0, 0xFC y 0x1E0). El diagnóstico básico solo consulta IORegistry; `--read-chip-info` envía una única lectura USB del registro 0xFC, sin escrituras ni carga de firmware.

## Ejecutar

Con Xcode Command Line Tools y Python 3:

```sh
make test
make probe
make universal
make receive-universal decode
./build/archer-decode tests/fixtures/synthetic-c2h-be.bin
./build/archer-probe-universal --descriptors
./build/archer-probe-universal --dump-descriptors /tmp/archer-new-config.bin
./build/archer-probe-universal --read-chip-info
./build/archer-probe-universal --read-startup-state
python3 tools/firmware.py extract-windows artifacts/windows/app/Driver/Win11/rtwlanu7.sys
python3 tools/inspect_inf.py /ruta/al/paquete/extraido
```

`probe` consulta IORegistry, muestra únicamente el adaptador objetivo y devuelve 1 si está ausente o 2 si falla la enumeración. El ejecutable universal contiene arm64 y x86_64; eso no sustituye pruebas en un Mac Intel físico.

`--descriptors` enumera los endpoints mediante IOUSBLib; `--dump-descriptors` guarda una configuración binaria en un archivo nuevo. `--read-chip-info` utiliza un timeout máximo solicitado de 1 segundo y comprueba la longitud recibida. macOS puede impedir estas operaciones dentro de un sandbox; requieren acceso USB del proceso. No usa apertura exclusiva ni cambia la configuración. Este transporte de diagnóstico IOUSBLib no sustituye la extensión DriverKit del producto final.

`register_request()` construye los ocho bytes del encabezado de control para registros, sin enviar solicitudes. Su dirección de 24 bits y anchuras 1/2/4 proceden de la implementación pública rtw89. No contiene secuencias de encendido, calibración ni firmware.

## Evidencia y propuesta

La tercera etapa recuperó dos imágenes embebidas del SYS oficial y añadió inspección estructural offline. Consultar [research/FIRMWARE.md](research/FIRMWARE.md) para hashes, reproducción y límites. No se ha cargado ninguna de estas imágenes al adaptador.

Consultar [research/FINDINGS.md](research/FINDINGS.md) y [research/PROPOSAL.md](research/PROPOSAL.md). Los paquetes originales se mantienen en `artifacts/`, excluidos de Git. No se incluye una licencia de redistribución para binarios TP-Link.

El INF oficial vincula este USB ID a `RTL8922au.ndi`. Esto identifica la familia que Windows selecciona; el modelo comercial exacto sigue sin comprobarse físicamente.

La cuarta etapa añade codificación H2C, envoltura USB BE y decodificación RX/C2H con pruebas. [research/FIRMWARE_CHANNEL.md](research/FIRMWARE_CHANNEL.md) documenta el intento de recepción real (dos timeouts, sin datos) y el comando de recepción acotada. Todavía no se envían comandos al firmware.

La quinta etapa logró una respuesta real del firmware mediante `GET_FEATURE` por buzón de registros. [research/REGISTER_MAILBOX.md](research/REGISTER_MAILBOX.md) documenta el protocolo y la prueba: 2 flujos TX/RX y 802.11be. `make mailbox-universal` compila el diagnóstico; `--query-with-io-window` escribe registros volátiles y restaura después el bit de acceso WLAN. Todavía no proporciona conexión Wi-Fi.

### Recepción bulk con ventana WLAN IO

`make receive-universal` y
`./build/archer-receive-universal --with-io-window NUEVO_DIRECTORIO` permiten
repetir la prueba acotada con restauración del acceso WLAN. La prueba física
ARM obtuvo respuesta PHY_CAP por registros, pero ambos endpoints bulk IN
agotaron su tiempo sin datos. Véase `research/bulk-io-window-observation.json`.
Todavía falta validar la inicialización y los comandos del transporte bulk.

### Estado de inicialización USB/MAC

`archer-mailbox-universal --inspect-transport` inspecciona las colas dentro de
la ventana WLAN. La prueba física confirma HCI/HAXI DMA habilitados, pero
agregación RX y CMAC deshabilitados. El mapa y las limitaciones están en
`research/TRANSPORT_INITIALIZATION.md`; no se modificaron registros DMA.

### Plan DLE USB2 sin acceso al hardware

`python3 tools/dle_plan.py` genera las cuotas y los campos del perfil normal
USB2 SCC. Valida los 589824 bytes de memoria y las cuotas H2C/C2H. El plan es
offline y todavía no ejecuta la transición del firmware ni inicializa Wi-Fi.

### Primer intercambio bulk confirmado

El adaptador recibió un LOG_CFG H2C de diagnóstico por USB y devolvió un
REC_ACK C2H coincidente en el Mac ARM. Captura real y límites en
`research/bulk-command-observation.json` y `research/FIRMWARE_CHANNEL.md`.
Se confirmó recepción del comando; todavía no hay radio, escaneo ni conexión.

El seguimiento de comandos ya validó físicamente REC_ACK y DONE_ACK exitoso
para LOG_CFG, rechazando un ACK pendiente de otra secuencia. Véase
`research/bulk-completion-observation.json`. Aún falta inicializar la radio.

### Sesiones y preparación de radio

La prueba bulk completa dos comandos y conserva las secuencias entre procesos
en `artifacts/37ad-0104-sequences.bin`. Ejecutar desde este proyecto y conservar
ese archivo: no hay reciclaje automático al agotar las 256 secuencias. Se
validaron físicamente las secuencias 2–5 con retorno 0. El decodificador efuse
offline y los requisitos pendientes están en `research/RADIO_BRINGUP.md`.
La radio todavía no se ha inicializado.

### Efuse de fábrica adquirida

`make efuse-universal` construye el lector DDV/PHY Intel y ARM. En el Mac ARM
se adquirieron ambos mapas y se verificó la restauración de sus controles.
El bloque RF contiene RFE tipo 1 y trim de cristal 0x42. Los decodificadores
son offline; esos parámetros todavía no se han aplicado a la radio. Evidencia
en `research/efuse-acquisition-evidence.json`.

### Tablas y calibración PHY identificadas

El decodificador PHY obtiene trims térmicos A=−1/B=0 y los códigos PA/PAD.
El analizador de firmware Linux localizó tablas BB, ganancias, radio A/B y
NCTL. No se ejecutaron esas tablas: falta validar el AID del chip y sus
condiciones. Consulte `research/RADIO_BRINGUP.md`, etapa 14.

### Identidad analógica validada

El lector XTAL-SI obtuvo AID 0x6933 y revisión analógica 1. La selección
offline ya filtra tablas por esa identidad y conserva sus índices de radio.
`make identity-universal` construye el lector Intel/ARM. Falta implementar
el intérprete de condiciones de tablas antes de aplicar registros RF.

### Intérprete PHY offline

`tools/phy_table.py` resuelve condiciones y retardos para RFE 1/CV 1. Sus
selecciones coinciden, par por par y en orden, con las funciones originales
Linux ejecutadas sin hardware. El plan todavía no escribe registros RF ni
inicializa la radio. Véase `research/phy-table-reference-check.json`.

### Ganancias BE y acceso RF-v2

`tools/bb_gain.py` decodifica las ganancias estructuradas; los arrays coinciden
con los callbacks originales Linux en 1512 posiciones. El helper C++
`include/archer/rf_access.hpp` calcula direcciones y comandos RF-v2/HWSI sin
ejecutarlos. La inicialización física BB/RF sigue pendiente.

### Estado físico BB y backend RF

La prueba temporal ya permite leer los registros BB sin valores de acceso
inválido. La consulta PHY_CAP y la identidad AID 0x6933 responden. La lectura
RF por HWSI aún agota el plazo: ni los controles de la tabla seleccionada ni
el reloj BB de 640 MHz bastaron para resolverla. Todos los controles
temporales se restauraron con lectura de verificación.

`make bb-probe-universal mailbox-state-universal` construye las herramientas.
`archer-bb-probe-universal --inspect-prerequisites` obtiene una captura fija
de controles dentro de la ventana BB temporal, sin lanzar comandos RF. Las
opciones `--table-hwsi-control` y `--clock-hwsi-control` son experimentos
explícitos, con valores iniciales restringidos y restauración. No constituyen
una inicialización BB/RF completa.

La lectura independiente de controles confirma BBMCU0 en reset y boot-ready
desactivado. Las fuentes y el archivo firmware ya tienen commits fijados; la
versión residente sigue desconocida. El siguiente paso es implementar una
secuencia explícita de arranque/carga, incluyendo BBMCU.
Resultados: `research/bringup-stage23-observation.json` y
`research/hwsi-stage24-observation.json`. Aún no hay escaneo ni conexión Wi-Fi.

### Inspección de firmware y BBMCU

`make firmware-state-universal` construye un inspector de cuatro registros,
sin escrituras ni comandos de buzón. Ejecuta
`./build/archer-firmware-state-universal` desde la raíz del proyecto.
`python3 tools/bbmcu.py artifacts/rtw8922a_fw-1.bin 1` selecciona y valida
estructuralmente los candidatos del archivo para CV=1: principal 0.35.41.0 y
BBMCU0 0.1.0.0. No los carga ni los identifica como firmware residente.
Véase `research/firmware-state-stage25-observation.json`.

### Recuperación después de reconectar

Estado actual: el dispositivo volvió a Wi-Fi `37ad:0104`; la causa exacta
del cambio no quedó establecida. Los párrafos siguientes registran los
intentos previos en modo instalador.

El adaptador reapareció como CD virtual Realtek `0bda:1a2b` (DISK), no como
Wi-Fi `37ad:0104`. Los helpers experimentales de expulsión compilan, pero
macOS bloqueó el acceso antes de enviar comandos. Se solicitó expulsión con
permisos de administrador del CD virtual identificado. No se ha validado aún
el cambio de modo; véase `research/reconnect-mode-observation.json`.

Si `diskutil` no encuentra el CD virtual, el helper experimental
`build/archer-optical-eject-universal --inspect` lo redescubre por IORegistry.
Su opción `--eject-installer` requiere sudo y usa DKIOCEJECT sobre el medio
Realtek verificado. La inspección está validada; la expulsión administrativa
todavía no se ha ejecutado ni ha confirmado el retorno al modo Wi-Fi.

La expulsión óptica fue aceptada, pero el adaptador volvió a DISK. El helper
USB directo ahora descubre los endpoints reales 0x84/0x05; aún falta probar
su apertura desde Terminal con sudo. Un status 0 de expulsión no confirma
el cambio de modo: debe reaparecer `37ad:0104`.

### Preparación de carga y trabajo pendiente

`./build/archer-efuse-universal --security-only NUEVO_DIRECTORIO` lee solo
cuatro bytes de selección de seguridad efuse, sin programación OTP. El
resultado físico actual es `ff ff ff ff`. El preparador offline se reproduce:

```sh
python3 tools/firmware_download.py artifacts/rtw8922a_fw-1.bin 1 artifacts/efuse-security-stage26/firmware-security.bin
```

Genera metadatos y hashes de 193 paquetes principales y 19 BBMCU; no reinicia
CPU ni envía firmware. El estado completo y criterios pendientes están en
[research/ROADMAP.md](research/ROADMAP.md).

### Ejecutor experimental de arranque

`make firmware-boot-universal` genera un bundle ligado al hash del firmware y
compila el ejecutor Intel/ARM. `--boot-and-read-rf` intenta reiniciar CPU y
cargar principal+BBMCU0 sobre el transporte USB ya preparado; todavía no es
un inicializador general MAC/DLE. El primer intento se detuvo en el acceso
DCPU 0x888, después del reset y antes de enviar firmware. Se requiere
recuperar el dispositivo antes de otro intento. El ejecutor actualizado
comprueba ese acceso antes de cualquier reset.

`archer-firmware-state-universal --inspect-boot-access` inspecciona los anchos
de acceso DCPU y controles fijos de transporte cuando el firmware original
está listo. Evidencia: `research/firmware-boot-stage27-observation.json`.
El hito de firmware/BBMCU y respuesta RF aún no está completado.

### Verificación del hito de firmware/RF

`make firmware-boot-universal bb-probe-universal` construye las herramientas
ligadas al firmware de referencia. El perfil experimental validado es
`./build/archer-firmware-boot-universal --boot-usb-no-dcpu`; requiere el estado
inicial USB/HAXI conocido y no reintenta cargas. Una vez cargado,
`--verify-loaded-rf` comprueba cuatro registros por ruta sin reset ni recarga.
La ruta predeterminada sigue rechazando DCPU inaccesible antes del reset.
No se debe repetir el arranque sobre un dispositivo ya inicializado: sus
precondiciones lo rechazan. Consulte las limitaciones y estado final en el
informe de etapa29; los apartados anteriores conservan la investigación.
