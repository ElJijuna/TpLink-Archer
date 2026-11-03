# Etapa96 — primer contraste simultáneo con receptor independiente

Fecha: 2026-11-02. No se repite el intento tag6.

El coordinador obtuvo handoff del receptor del Mac y ejecutó el sondeo único.
USB aceptó104 bytes en endpoint5; firmware devolvió RETRY_LIMIT/tag6/count1.
IFS-CLM contó0 unidades TX en baseline y17 unidades en la ventana activa
(544us internos). La limpieza fue confirmada y USB drenado: contexto CLEAN,
TX pausado y197 secuencias disponibles. El marcador tag6 permanece consumido.

La captura externa contiene2477 paquetes durante45.978s, todos con metadatos
de2452MHz. Incluye381 beacons de Especial sin marca radiotap bad-fcs, con
señal−34 a−43dBm;35 ACK y151 CTS sin esa marca. Hay1014 paquetes marcados
bad-fcs en el conjunto. Ninguna línea decodificada contiene la MAC de prueba
02:00:00:89:22:a1. No se realizó validación CRC software del PCAP externo.

El auditor de la recepción del propio Archer encontró5 ACK y26 CTS con CRC
software válido; ninguno dirigido a la MAC de prueba. Esto confirma recepción
de controles ajenos durante la prueba, sin atribuir su transmisor ni AP.

No hay evidencia externa de una trama reconocible del Archer. La ausencia
no demuestra ausencia de energía RF: quedan abiertas señal no decodificable,
ruta/potencia TX, configuración/calibración y pérdidas del receptor. El AP se
observa con señal fuerte, pero eso no mide la potencia transmitida por el Archer.
El contador interno no sustituye la comprobación en antena.

El código1 del helper de monitor es de restauración, no de tcpdump:
capture_exit0,0 descartes del kernel; restauración exacta y alternativa20MHz
rechazadas con−3903. Debe reconectarse Wi-Fi manualmente. La alternativa no
resolvió el problema y no se presenta como recuperación validada.

Siguiente investigación: revisar configuración de ruta/potencia TX y semánticas
RF pendientes con evidencia de fuente/binario antes de otro intento. No
reintentar el tag6 ni cambiar registros a ciegas. Avance ponderado52 %.

Evidencia con hashes: [external-probe-stage96-observation.json](external-probe-stage96-observation.json).
Capturas y logs locales en artifacts/external-probe-stage95/.
