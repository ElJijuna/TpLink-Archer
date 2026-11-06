# Etapa109 — recuperación fría y contexto verificados

Captura2026-11-04; análisis2026-11-05. Tras la expulsión local del CD, una orden SCSI pudo enviarse al instalador. El helper no obtuvo CSW válido y terminó3; la enumeración posterior sí confirmó37ad:0104. No se presenta la respuesta SCSI como éxito. La inspección independiente confirmó BB reset0 y `cold_profile=true`.

La continuación de la transición107 encontró un error del coordinador: reutilizaba el directorio de captura `reload`, ya creado por el preflight anterior. El helper rechazó esa ruta antes de abrir USB. Una regresión que crea realmente el directorio reprodujo el fallo. Se corrigió el uso de rutas separadas y se permitió continuar únicamente desde el rechazo exacto preservado (stdout vacío, error de directorio exacto, hashes originales intactos). Se conservaron los logs de cada rechazo y el ledger anterior.

La continuación corregida verificó la recarga completa:212 escrituras de descarga correctas, cero H2C y callbacks drenados. Se archivó el ledger113 usadas/143 libres de la época anterior. La nueva época se confirmó con254 secuencias disponibles y contexto/fence CLEAN, sin borrar marcadores de ensayos Wi-Fi.

El bootstrap frío completó radio, RFK inicial/TX y escaneo pasivo:21 redes,372 gestiones aceptadas y0 FCS erróneos, con replay igual. Quedaron232 secuencias. El perfil canal9 y sus ocho terminales de calibración se restauraron, dejando224. DPK/TSSI mantienen sus límites de interpretación previos.

Se comprobó específicamente el paso que había fallado: contexto pasivo en9, secuencias32–40, ocho ACK exigidos en pasos0/1/2/3/4/6/7/8; paso5 es entrega sin ACK según el protocolo existente. El contexto llegó a retirada confirmada y terminó CLEAN. Captura:618 transferencias,564 paquetes normales,390 beacons. TX de red siempre pausado. Quedaron215 secuencias.

El postflight validó210 registros residentes,0 diferencias, concesión original0x01775774 y TX pausado. No hubo nuevas tramas Wi-Fi ni WPA2 en esta recuperación. El contraste muestra que el canal de control volvió a funcionar tras el arranque frío; no identifica la causa precisa de su fallo anterior.

Este es un estado histórico comprobado. La etapa110 observó posteriormente modo instalador, por lo que no se debe asumir que esta generación de firmware siga activa.

Evidencia: [observación109 y hashes](recovery-stage109-observation.json).
