# Etapa172 — una retransmisión REQUEST acotada

2026-11-24. La auditoría171 autenticó22 tramas sin rechazos y reprodujo REQUEST; el AP reflejó REQUEST con XID/chaddr coincidentes. No hay DHCPACK ni DHCPNAK capturados. ACQ ya produjo TX_DONE de datos en170; no hay evidencia de que la ausencia del ACK sea rechazo del parser. La causa de la falta de respuesta sigue sin determinarse.

Se implementó una única retransmisión REQUEST dentro del mismo ensayo, conservando XID/servidor/dirección y generando PN y secuencia Wi-Fi nuevos. Solo se permite tras TX_DONE/count1 del primer REQUEST. Espera inicial aleatoria3–5s; el segundo envío usa el resto del presupuesto REQUEST total10s, recortado también por el plazo global. Se vuelve a drenar RX antes de decidir retransmitir para evitar enviar si ya llegó ACK/NAK. No retransmite después de un NAK autenticado del servidor elegido. No reinicia el ensayo, ni repite paquetes cifrados, ni permite una segunda retransmisión REQUEST.

La retransmisión tras ausencia de ACK/NAK corresponde a [RFC2131, secciones3.1 y4.1](https://www.rfc-editor.org/rfc/rfc2131.html). Este ensayo conserva límites más acotados que un cliente de producción; no implementa reinicio automático a INIT.

Layoutv2 registra M2=2, M4=6, DISCOVER=7, REQUEST=9, retryREQUEST=10, RELEASE=11 y deauth=12; slots Wi-Fi100–112. Tags de reporte12/11/10/13/14 respectivamente para DISCOVER/REQUEST/retry/RELEASE/deauth. Los validadores exigen los reportes y su cronología. El auditor acepta layouts históricos yv2, verifica REQUEST retransmitido y PN creciente, y sigue sin escribir claves ni plaintext.

La regresión cubre estados/prohibición de segundo retry, ACK posterior al retry, NAK y servidor/transacción coincidentes, reportes ausentes/fallidos, metadatos frescos y las combinaciones de slots opcionales; también corrupción cifrada de REQUEST/retry/RELEASE. Pasaron134 pruebas Python, suites C++ y compilación ARM/Intel. Esto no prueba el efecto por radio.

Contexto170 sigue DIRTY. La recuperación173 y el intento174 todavía no están preparados; la admisión permanece bloqueada y no se publicó comando de ensayo. Esperando desconexión física informada por el usuario para verificar ausencia. Conservar reservas170 y resultados; no ejecutar calibraciones ni ensayos anteriores.

[Preparación y hashes](request-retry-stage172-observation.json).
