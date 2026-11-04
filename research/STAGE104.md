# Etapa104 — recepción nativa del mensaje WPA2 M1

Captura: 2026-11-03; análisis y postflight:2026-11-04. Tags nuevos13–15 completaron autenticación, asociación con estado0/AID2 y desautenticación, con TX_DONE/count1. La ventana temporal permitió datos RX durante hasta3 segundos tras asociarse.

El auditor nativo encontró un M1 RSN de Especial dirigido a nuestra estación: `rx-164.bin`, offset48,135 bytes con FCS válido. Descriptor2, key_info0x008a, longitud de clave16, replay1, ANonce no nulo, MIC cero y sin key data. El auditor procesó731 transferencias y727 candidatos normales. La fixture preserva exactamente esos135 bytes.

La captura independiente contiene6417 registros:5511 CRC válidos,905 inválidos,1 truncado,397 beacons válidos y10 tramas válidas relacionadas con nuestra MAC. Corrobora gestión, pero **no contiene un M1 independiente válido**. La prueba del M1 es nativa.

Concesión y filtros restaurados, contexto CLEAN,152 secuencias disponibles, TX pausado. El postflight validó210 registros contra el plan residente sin H2C ni TX. Suites C++ y113 pruebas Python aprobadas en esta etapa.

No se obtuvo contraseña, no se transmitieron M2/M4 y no se estableció seguridad ni un camino de datos cifrados. Evidencia: [observación y hashes](eapol-rx-stage104-observation.json).
