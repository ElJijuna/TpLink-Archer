# Etapa114 — perfil frío no observado

2026-11-06. Tras la indicación de continuar se verificó37ad:0104. La inspección de solo lectura mostró BB control0x80c30007, BB reset0x7ff97ff9 y firmware0x0c000022: `cold_profile=false`.

El ledger permanece idéntico al cierre113:50 usadas/206 libres, identidad de época3c61dfdb-22b1-462b-8f28-473ac2b0314b. Contexto DIRTY y barrera de transición CLEAN. No se inició el coordinador de recuperación: así no se publica una barrera DIRTY antes de saber que el perfil frío está presente. No se enviaron H2C, tramas Wi-Fi ni resets. Los marcadores anteriores no cambiaron.

La corrección SW_MLD y la instrumentación PHY siguen solo compiladas/probadas localmente, sin nueva validación por radio. Para avanzar se requiere completar la desconexión física, esperar5 segundos, reconectar y expulsar el CD Realtek con el helper verificado en Terminal; después se inspeccionará de nuevo el estado antes de iniciar una frontera de firmware. No se trata una respuesta de continuación como prueba de que ocurrió la desconexión.

Evidencia: [observación114](reconnect-stage114-observation.json).
