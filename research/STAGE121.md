# Etapa121 — recuperación fría verificada

2026-11-06. Tras informar el usuario de la expulsión óptica, la enumeración todavía mostraba el instalador0bda:1a2b. Una única orden SCSI de expulsión abrió la interfaz y fue aceptada, pero no obtuvo CSW válido (salida3). La enumeración posterior confirmó Wi-Fi37ad:0104 y el inspector confirmó `cold_profile=true`, BB reset0. Estas comprobaciones acreditan el cambio de modo, sin tratar el CSW como válido.

La transición controlada archivó el contexto DIRTY, el ledger anterior50/206 y su identidad. Verificó212 escrituras de firmware, estado listo, cero H2C y drenaje antes de publicar la época nueva `7184d1e9-6f13-45a3-981a-635cadffdb33`, contexto/fence CLEAN y254 secuencias libres.

El bootstrap frío completó radio, RFK y escaneo pasivo:15 redes. Se restauró canal9 y terminaron las calibraciones del perfil. Una invocación equivocada del comprobador de canal1 se detuvo ante el canal distinto antes de reservar secuencias o enviar H2C; se conserva su evidencia. Después, el comprobador específico de canal9 creó y retiró el contexto correctamente: ocho ACK,364 transferencias,282 paquetes normales y202 beacons. Postflight:210 registros coincidentes y TX de red pausado. Las semánticas DPK/TSSI y calidad RF completa siguen pendientes.

Estado final: contexto/fence CLEAN y215 secuencias libres; ningún envío nuevo de trama Wi-Fi. El marcador116 consumido y la evidencia anterior se conservan. La recuperación no prueba DHCP ni recepción unicast.

[Observación y hashes](recovery-stage121-observation.json).
