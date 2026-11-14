# Etapa154 — recarga fría correcta; parada antes de RFK de canal9

2026-11-17. Tras ausencia152 y nueva sesión instalador153, la orden SCSI no obtuvo CSW válido. La enumeración posterior confirmó37ad:0104 y el inspector confirmó perfil frío, BB reset0. Recarga fría completada y época nueva publicada; bootstrap frío también completó recepción pasiva.

La restauración host de canal9 terminó, pero el primer drenaje previo a RFK no encontró entrada silenciosa. Se detuvo antes de reservar secuencia o enviar H2C de RFK: cero registros `rfk_command`, cero archivos `tx-*.bin` en la captura de calibración y ledger24 usadas/232 libres, igual al cierre del bootstrap. No se transmitió Wi-Fi nuevo ni se ejecutó el contexto pasivo posterior. Contexto DIRTY; no se declara recuperación completa.

Postflight posterior:210 registros coincidentes y TX pausado. Esto comprueba configuración host/pausa, no sustituye las calibraciones ni los ACK de contexto faltantes. El mensaje genérico del helper pide no repetir RFK; aunque no se observó envío pendiente, se conserva la barrera DIRTY y no se autoriza reutilización directa.

El drenaje de TX RFK seguía usando polls de5 ms para ambos endpoints y un máximo512 vueltas: esperar en el endpoint inactivo puede impedir consumir una cola finita del otro dentro de3s. Se sustituyó por `drain_input_until_quiet`, ya usado en la ruta de contexto: polls sin bloqueo mientras haya datos, con espera solo para confirmar silencio. Una regresión reproduce el fallo con la pauta5ms y verifica el drenaje del mismo backlog. La nueva versión no se ha probado físicamente en este punto porque el contexto quedó DIRTY.

Hace falta una frontera fría nueva antes de repetir la recuperación con el helper corregido. No ejecutar directamente calibración o ensayos antiguos ni borrar reservas.

[Observación y hashes](recovery-stage154-observation.json).
