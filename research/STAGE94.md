# Etapa94 — baseline independiente confirmado en canal9

Fecha: 2026-11-02.

El usuario ejecutó el capturador con sudo. tcpdump informó1499 paquetes,
1696 recibidos por el filtro,0 descartados por el kernel y exit0. El archivo
PCAP contiene1499 registros decodificados por tcpdump, todos a2452MHz.

El AP Especial, BSSID50:92:6a:11:06:66, aparece en165 beacons sin marca
radiotap bad-fcs, entre−36 y−40dBm. Se observaron16 ACK y72 CTS sin esa
marca. Hay631 paquetes marcados bad-fcs en el conjunto completo. No se
realizó auditoría CRC software: ausencia de marca no equivale a CRC verificado.
No aparece la MAC de prueba y no hubo TX nuevo del Archer durante el baseline.

La captura finalizó correctamente, pero restaurar el canal36 original devolvió
kA11NotSupportedErr(−3903). Se indicó reconectar la red manualmente. No se
ha demostrado la causa del rechazo. El helper incorpora ahora un intento
alternativo de recuperar la frecuencia original en20MHz si el perfil original
es rechazado; conserva el resultado de error de restauración exacta y no afirma
haber asociado a la red. La alternativa compila, pero no se ejecutó físicamente.

El receptor independiente está validado para el baseline pasivo. La siguiente
prueba requiere una captura simultánea y un sondeo nuevo con marcador propio;
el tag5 consumido no se reutiliza. Una captura pasiva anterior no demuestra
emisión del Archer ni garantiza el canal de una captura futura. Avance52 %.

Evidencia: [monitor-baseline-stage94-observation.json](monitor-baseline-stage94-observation.json).
PCAP local: artifacts/monitor-channel9-baseline/monitor-channel9.pcap.
Decodificación: artifacts/monitor-channel9-baseline-decoded.txt.
