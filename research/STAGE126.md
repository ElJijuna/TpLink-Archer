# Etapa126 — frontera física todavía no observada

2026-11-06. Tras confirmar el usuario la reconexión, el Mac enumera37ad:0104, pero mantiene exactamente la identidad IORegistry0x100258009 y sessionID13471290465223 registradas en121. La lectura actual vuelve a indicar `cold_profile=false`, BB reset2147057657 y firmware control201326626.

No se ha observado una nueva enumeración o frontera fría. No se inició recarga de firmware ni H2C/TX nuevo. Contexto DIRTY y206 secuencias libres conservados. No se atribuye la causa a una acción concreta del usuario; la evidencia del Mac no acredita todavía la interrupción necesaria.

La siguiente comprobación debe hacerse en dos pasos: retirar físicamente el Archer y dejarlo desconectado mientras se comprueba su ausencia; solo después volver a conectarlo y comprobar nueva enumeración/perfil. Esto permite verificar la frontera directamente en lugar de repetir confirmaciones de reconexión. No ejecutar WPA2 ni borrar reservas.

[Evidencia y hashes](recovery-stage126-observation.json).
