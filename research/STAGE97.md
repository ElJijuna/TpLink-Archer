# Etapa97 — auditoría física de ruta y potencia TX

Fecha: 2026-11-03. Canal9, Mac ARM. Sin nuevo sondeo.

Se añadió archer-tx-power-audit-universal --paused-channel9. Valida el perfil
firmware/ADDA/BB, scheduler pausado, canal9 y MCC0 antes de leer. Contrasta
la combinación final de máscaras de la ruta AB y las197 escrituras del perfil
de potencia world/unconfigured. Los registros se leen sin reprogramarlos;
solo se abre/restaura la ventana de acceso WLAN estándar. No envía H2C,
no reinicia firmware, no recalibra ni reserva secuencias.

Dos snapshots físicos coincidieron en los210 registros únicos de ruta y
potencia, con0 diferencias. Esto reduce la hipótesis de una tabla instalada
que haya perdido sus valores. No valida que el plan del host sea suficiente
para transmitir correctamente ni certifica potencia o calidad RF.

La referencia local rtw8922a.c:1868–1917 define las siete palabras de ruta AB
que coinciden con el equipo. Su set_txpwr_ref:2429–2442 establece referencias
MAC OFDM/CCK en0; set_txpwr_diff:2454–2496 parte de referencia0 para CV1
(solo CAV utiliza16), con K base18. Por tanto, referencias0 no son por sí
solas evidencia de una omisión. La primera palabra byrate CCK leída fue
0x28282828 y la primera palabra de límites0x000e001d, coincidentes con el
plan ligado al firmware. No se convierten aquí en potencia física medida.

Lecturas adicionales: PMAC_GNT físico0x20980=0x5250; TSSI_PWR A/B
0x2e610/0x2e710=3. Bit3 está limpio en ambos, coherente con cont_en(true)
de rtw8922a_rfk.c:12–22 y la salida del reset de rtw8922a.c:2315.
No se observó el bloqueo de continuidad TSSI de esa función concreta.
Esto no interpreta los estados de alineación ni valida TSSI global.

CCK_TXEN/TXON crudos0x22e00/0x22e04 fueron0. No permiten atribución al
sondeo anterior: macos_context_rx.cpp resetea los contadores diagnósticos
después de la ventana del sondeo (reset_rx_diagnostic_counters, posterior al
terminal TX). No se usa ese0 como prueba de ausencia de transmisión.

Estado final: contexto CLEAN,197 secuencias, mismo digest del ledger que tras
el tag6, TX pausado. Build Intel/ARM aprobado; Intel físico pendiente.
La siguiente instrumentación deberá tomar contadores PHY antes y después
del envío, antes de su reset, y contrastarlos con captura externa. No se ha
creado otro intento TX ni se han cambiado potencia/antenas. Avance52 %.

Evidencia: [tx-power-stage97-observation.json](tx-power-stage97-observation.json).
Se conserva el ejecutable exacto del segundo snapshot y hashes de fuentes,
plan generado, referencia y logs. Los artefactos locales no forman parte
del checkout distribuido.
