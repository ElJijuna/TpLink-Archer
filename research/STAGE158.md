# Etapa158 — recuperación completa con drenaje RFK corregido

2026-11-17. Tras ausencia156 y nueva sesión instalador157, una orden SCSI única terminó sin CSW válido. La enumeración posterior confirmó37ad:0104 y los registros confirmaron perfil frío, BB reset0. Se utiliza esa evidencia, no el CSW, para acreditar el cambio de modo.

La transición archivó el contexto DIRTY y el ledger24/232 de la parada154. Verificó212 escrituras de firmware, estado listo, cero H2C y drenaje antes de publicar época018e8cb1-df19-4467-a428-b824e63711f3.

Bootstrap frío completo:18 redes pasivas. El calibrador con drenaje corregido completó los ocho comandos RFK de canal9. Es una ejecución física satisfactoria del nuevo drenaje; no demuestra por sí sola causalidad exclusiva del pacing en la parada154 ni valida plenamente DPK/TSSI o calidad RF.

Contexto pasivo creado/retirado con ocho ACK:676 transferencias,662 paquetes normales y212 beacons. Postflight210 registros coincidentes, TX pausado, contexto/fence CLEAN y215 secuencias libres. No hubo nuevas tramas Wi-Fi de ensayo. Marcadores históricos conservados.

[Observación y hashes](recovery-stage158-observation.json).
