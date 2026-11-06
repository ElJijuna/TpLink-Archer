# Etapa129 — recuperación después de ausencia USB verificada

2026-11-06. Tras la ausencia comprobada127 y la sesión nueva128, el usuario aportó DKIOCEJECT0 del instalador. La enumeración aún mostraba0bda:1a2b. Una única orden SCSI de expulsión no obtuvo CSW válido (salida3); después de la transición, la enumeración confirmó37ad:0104 y los registros confirmaron perfil frío con BB reset0. No se atribuye validez al CSW.

La recuperación archivó el contexto DIRTY, ledger50/206 e identidad anteriores. Verificó212 escrituras de firmware, estado listo, cero H2C y drenaje antes de confirmar nueva época51e59f80-528b-446c-b79f-3c9ac307bca2 y254 secuencias libres.

Bootstrap frío completado:16 redes en recepción pasiva. Canal9/calibraciones restaurados. El contexto pasivo recibió ocho ACK y confirmó retirada:492 transferencias,470 paquetes normales y244 beacons. Postflight210 registros correcto, TX pausado. Estado final CLEAN,215 secuencias libres; ninguna trama Wi-Fi nueva. Los marcadores116/122 y sus pruebas se conservan.

No demuestra DHCP o recepción unicast, ni resuelve las semánticas DPK/TSSI o calidad RF completa.

[Observación y hashes](recovery-stage129-observation.json).
