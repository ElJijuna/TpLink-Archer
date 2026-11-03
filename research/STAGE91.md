# Etapa91 — recepción pasiva de ACK y CTS

Fecha: 2026-11-02. Canal9, perfil USB2/SCC, Mac ARM.

Se añadió un perfil de recepción que entrega al host los subtipos ACK y CTS:
CTRL_FLTR 0x11424 = 0x3000, gestión completa y datos deshabilitados. La
ventana conserva y restaura sus controles. Este filtro decide entrega al
host; no se ha demostrado que determine la decisión de ACK del motor MAC.

La comprobación inicial encontró que el perfil de firmware anterior ya no
estaba cargado: 0x1e0 era 0x0c000000, frente a 0x0c000022 esperado. Se creó
una nueva época verificada, se cargó firmware, se ejecutó el arranque de radio
y se repitió la calibración del canal9, con ocho terminales OK. Esto no
resuelve las semánticas pendientes de calidad DPK/TSSI.

La captura pasiva terminó con362 transferencias,306 paquetes normales,
283 beacons y replay idéntico al análisis en vivo. El auditor nuevo comprobó
2 ACK y1 CTS mediante FCS software, sin controles con FCS inválido. Ninguno
iba dirigido a la MAC de prueba. Las tres direcciones receptoras eran
distintas; ACK/CTS no identifican por sí solos al transmisor ni al BSSID.
No se envió ningún sondeo activo en esta captura.

Estado final de esta etapa: contexto CLEAN,215 secuencias disponibles y TX
pausado. Evidencia con hashes: [control-rx-stage91-observation.json](control-rx-stage91-observation.json).
Los binarios y capturas crudas se conservan localmente en artifacts/control-rx-capture-stage91-evidence/.
