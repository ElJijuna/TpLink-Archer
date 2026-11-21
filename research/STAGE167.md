# Etapa167 — recuperación detenida por IQK RX

2026-11-24. La expulsión local no produjo interfaz Wi-Fi inmediatamente. La interfaz instalador fue inspeccionada y se envió una expulsión USB única; aunque no llegó CSW válido, la inspección posterior confirmó Wi-Fi y perfil frío. La transición verificó firmware y creó una época nueva, archivando el contexto DIRTY anterior. Bootstrap pasivo completado con16 redes.

La calibración de canal9 completó pre-notify/TSSI/pre-notify/TXGAPK, pero se detuvo después del reporte IQK. El replay de rx-17.bin confirma TX fail[0,0], RX fail[0,1], ambos canales9, PHY0 y versión85. El flag RX de ruta B impide continuar: no ejecutar TSSI/DPK/RXDCK posteriores ni contexto de ensayo. No se relanza una calibración incierta ni se omite el flag. Callbacks USB drenados a cero; contexto DIRTY y fence CLEAN.

No se ejecutó el contraste ACQ ni un nuevo ensayo DHCP. La preparación168 no está publicada y la admisión permanece bloqueada por estado de recuperación. El cambio ACQ continúa compilado y validado como contraste, sin efecto RF comprobado. Hace falta una nueva frontera fría verificada antes de otra recuperación; conservar todos los resultados y reservas.

[Observación y hashes](recovery-stage167-observation.json).
