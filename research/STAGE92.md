# Etapa92 — sondeo unicast con recepción de controles

Fecha: 2026-11-02. Canal9, SSID Especial.

Se ejecutó exactamente un sondeo unicast de104 bytes mediante endpoint5,
con secuencia5/tag5, tasa CCK1 y RTS_EN desactivado, conservando el resto del
perfil anterior. El ejecutor exige el baseline pasivo de etapa91 y sus hashes,
el ledger correspondiente y un marcador de intento nuevo. No permite repetir
el intento consumido.

USB aceptó la escritura. El firmware devolvió RETRY_LIMIT, tag5, count1.
El monitor IFS-CLM contó TX0 durante el baseline y17 unidades de32us
(544us internos) durante la ventana activa. Ese contador no demuestra que
la señal salga correctamente por la antena.

La captura incluyó435 transferencias,361 paquetes normales y335 beacons.
El auditor de gestión aceptó350 tramas con FCS válido, incluyendo67 beacons
del objetivo, pero ninguna respuesta del objetivo a nuestro sondeo ni dirigida
a nuestra MAC. El auditor de control encontró0 ACK y0 CTS. El filtro CTRL
se verificó como0x3000 durante la ventana y se restauró posteriormente.
Habilitar su entrega al host no resolvió el fallo en esta ejecución.

La limpieza y drenaje USB terminaron correctamente: contexto CLEAN,
206 secuencias disponibles y TX pausado. El marcador de intento se conserva;
el fallo terminal de transmisión no se presenta como éxito de comunicación.

Validación: suites C++ y92 pruebas Python aprobadas; compilación universal
del capturador, auditor y sondeo nuevos, y de los perfiles de regresión
context-rx y special-monitored-probe. Sigue pendiente una captura independiente
por aire para distinguir emisión incorrecta, recepción del ACK y comportamiento
del AP. No hay asociación ni interfaz de red funcional. Avance ponderado52 %.

Evidencia con hashes: [ack-probe-stage92-observation.json](ack-probe-stage92-observation.json).
Captura y ejecutable exacto conservados localmente en artifacts/special-ack-probe-stage92/.
