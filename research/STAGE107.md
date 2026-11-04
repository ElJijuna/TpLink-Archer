# Etapa107 — estado frío aún no observado

2026-11-04. Tras la respuesta del usuario se comprobó enumeración37ad:0104. El coordinador existente de recuperación fría archivó el contexto DIRTY y el ledger113 usadas/143 libres y colocó la barrera de transición DIRTY. Su comprobación de perfil frío falló **antes de reset CPU, carga de firmware o H2C**: `Cold BB reset profile absent`. El ledger original permanece idéntico al archivo archivado. No se liberó el contexto ni se reintentó WPA2.

La inspección independiente posterior, que funciona con la barrera activa sin reservar secuencias, leyó BB control0x80c30007, BB reset0x7ff97ff9 y firmware0x0c000022. Resultado `cold_profile=false`: los registros siguen correspondiendo al perfil cargado. Esto no identifica por qué no se perdió ese estado; no se presupone que una confirmación verbal equivalga a un reinicio eléctrico observado.

Se preparó `--resume-cold-preflight` en el coordinador. Solo continúa una vez desde el rechazo exacto preservado, con hashes de los logs intactos, operaciones limitadas a lecturas y protocolo WLAN/XTAL de lectura restaurado, sin ninguna fase de reset/carga ni envío. Conserva logs originales y archivos anteriores, exige que contexto y ledger coincidan bajo locks, mantiene DIRTY durante la continuación y solo publica la nueva época tras verificar la frontera completa existente. No habilita el reenvío de comandos ni de tramas anteriores. No se ha ejecutado esta continuación.

Pruebas nuevas: traza física real de preflight, rechazo de cualquier reset/envío/escritura adicional, fallo de continuación que conserva ledger/DIRTY y éxito simulado que solo confirma tras la frontera completa. Pasan suites C++ y118 pruebas Python; inspector ARM/Intel compilado.

Siguiente paso: desconectar físicamente el adaptador de su alimentación USB durante unos segundos y reconectarlo. Primero se repetirá únicamente la inspección de enumeración/estado frío; no se ejecutará la continuación si sigue cargado. La barrera y las reservas no se borran manualmente.

Evidencia: [observación107](recovery-stage107-observation.json). Estado actual: contexto DIRTY, transición DIRTY, ledger anterior conservado,143 secuencias libres y ninguna trama Wi-Fi nueva.
