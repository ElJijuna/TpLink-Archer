# Etapa155 — coordinador supplicant validado; admisión bloqueada por recuperación

2026-11-17. Se añadió `--dhcp-supplicant` al coordinador, con selección del binario y marcador propios y manifiesto de esquema nuevo. El validador exige cero–tres respuestas M2 adicionales ante M1/replay crecientes, antes de M3 y M4; conserva OFFER/ACK coincidentes, RELEASE y desautenticación con reportes separados y una retransmisión DISCOVER como máximo.

El esquema reserva slots opcionales3–5 para M2 adicionales, M4 en6, DISCOVER en7, retransmisión DHCP en8, REQUEST9, RELEASE10 y desautenticación11. Cada envío usa secuencia distinta. La captura nueva escribe un identificador de layout; el auditor reconoce ese layout y conserva compatibilidad con capturas históricas. No utiliza el layout como prueba de MIC: esa prueba sigue realizándose criptográficamente.

El límite nativo final es30s, no35s del prototipo151. Para este modo, la captura debe seguir viva y la entrega al nativo ocurrir antes de8s desde su inicio; la captura conserva máximo45s. La gracia3s es exclusiva de desautenticación. Las esperas de fase pueden acortarse por el límite global. No se afirma que todos los presupuestos máximos individuales quepan simultáneamente.

Pruebas de los ocho caminos opcionales, evidencia ausente/duplicada, ANonce/replay, orden M3/M4, reportes y layout; regresión del drenaje RFK previo. Suites C++ y130 pruebas Python aprobadas. Binario supplicant, calibrador corregido y auditor compilan ARM/Intel.

**No hay un ensayo155 admitido.** La recuperación154 se detuvo antes de RFK y del contexto pasivo; contexto DIRTY y232 secuencias libres. Se publicó preparación `blocked_recovery_not_prepared`, y la admisión real rechazó ese estado sin acceder al USB o consumir marcador. No hay comando de ejecución autorizado para este estado. El nuevo modo aún no tiene efecto por radio demostrado.

La siguiente acción es retirar el Archer y dejarlo desconectado para comprobar ausencia antes de reconectar y recuperar con el drenaje corregido. No repetir148 ni ejecutar calibración directamente. Cero tramas Wi-Fi nuevas en esta etapa; avance físico64 %, tarea abierta.

[Validación y hashes](supplicant-stage155-observation.json).
