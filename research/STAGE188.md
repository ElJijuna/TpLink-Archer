# Etapas187–188 — USB Wi-Fi y recuperación fría verificados

El contraste186 realizó una expulsión DKIOCEJECT aceptada, seguida de tres sesiones instalador y sin Wi-Fi observado durante20s. En187 se envió una sola orden USB START STOP UNIT del helper existente; recibió un paquete con forma de sense y no un CSW válido. No se declara éxito SCSI. La observación independiente registró ausencia y después37ad:0104, estable durante el resto de la ventana y en las verificaciones posteriores. No se repitió la orden.

La recuperación188 verificó el perfil frío y completó212 escrituras de firmware sin H2C, archivando ledger/contexto/identidad anteriores antes de establecer la nueva época. Bootstrap, calibración de canal9 con flags IQK limpios y contexto pasivo completaron. Especial observado con72 beacons y1 probe response. Ocho ACK de retirada, drenaje y postflight210 verificados; contexto/fence CLEAN,215 secuencias libres. No se envió asociación ni DHCP durante esta recuperación; las calibraciones pueden incluir actividad RF interna.

La preparación182 quedó sustituida sin ejecución. El nuevo189 repite el contraste IP previsto dentro de la época actual, con marcador nuevo y admisión basada en recuperación188. No crea interfaz macOS. El fallo del cambio de modo por DKIOCEJECT no está reparado; el contraste USB directo produjo el modo Wi-Fi observado en esta ejecución.

Evidencia: [recovery-stage188-observation.json](recovery-stage188-observation.json), `artifacts/usb-switch-stage187/timeline.jsonl` y [supplicant-stage189-observation.json](supplicant-stage189-observation.json).
