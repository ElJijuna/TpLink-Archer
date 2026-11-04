# Etapa106 — intento detenido antes de autenticación/WPA2

2026-11-04. El usuario ejecutó el intento único preparado en105. El primer H2C de contexto (step0, secuencia104, desbloqueo/activación MACID) se entregó por USB:288 bytes, status0. No llegó su receive ACK durante el plazo de5 segundos. El ejecutor terminó2 con `Context command uncertain`; no reenvió ni comenzó retirada automática del contexto.

**No se transmitieron tramas de autenticación, asociación, EAPOL ni DHCP. La contraseña no fue puesta a prueba.** La concesión especial y la ventana de datos de la fase WPA2 tampoco llegaron a abrirse. No hay resultados nuevos sobre el funcionamiento de M2/M4 o CCMP.

Las nueve secuencias104–112 quedaron consumidas por la reserva inicial. Ledger113 usadas/143 libres, contexto DIRTY, transición de época CLEAN. Se conserva el marcador del intento completo (incluidas secuencias802.11 16–21 y tag0), aun cuando la parte Wi-Fi no se ejecutó. No se debe volver a ejecutar el comando105 ni borrar reservas para intentarlo.

El análisis offline encuentra930 transferencias nativas, todas con packet_type1 (reportes PHY), sin candidatos normales ni C2H. Todas se recibieron antes del primer H2C; tras él no hubo nuevas completions normales (solo abortos al cerrar). La solicitud preservada difiere de la primera solicitud de la etapa104 únicamente en el byte de secuencia95→104. Esto descarta un ACK presente que el parser de sesión hubiera ignorado, pero no distingue entre firmware sin procesar el H2C y ACK no entregado por USB.

La captura independiente tiene2396 registros,1532 CRC válidos,864 inválidos,392 beacons válidos de Especial y0 tramas válidas relacionadas con nuestra MAC. Todo a2452MHz. `monitor_exit1` corresponde al helper externo; no explica por sí mismo el fallo nativo. No se afirma que la radio haya dejado de funcionar: este intento no alcanzó el envío Wi-Fi.

Hipótesis pendientes: canal firmware/H2C sin respuesta; ruta de entrega USB/C2H detenida; cambio de estado persistente que las condiciones de preflight no detectaron. La lectura física posterior confirmó firmware/canal9, MCC0 y TX pausado, sin H2C ni resets. Una captura USB pasiva adicional de2s obtuvo0 transferencias y0 C2H; no apareció ACK tardío. No se cambia código de transporte ni se declara una corrección sin evidencia adicional. No se ha recuperado ni marcado CLEAN el contexto incierto.

Evidencia con hashes de cada archivo original: [observación106](wpa2-stage106-observation.json). Auditor CRC y auditor C++ offline preservados en artifacts. Estado105 conservado como registro de preparación; no representa el estado físico posterior a este intento.

La recuperación caliente ya falló en las etapas76/77 y el arranque eléctrico recuperó RFK en78. No se repite ese reset caliente como supuesto arreglo. El siguiente paso requiere desconexión/reconexión física del adaptador, inspección del estado frío y la frontera de recarga verificada existente. No se liberará DIRTY ni se reemplazará el ledger por editar archivos; la época anterior debe archivarse y el cambio depende de la frontera comprobada de firmware. Ningún marcador Wi-Fi se elimina.
