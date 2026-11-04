# Etapa102 — emisión y respuesta de Especial verificadas

Captura: 2026-11-03. El intento único tag9 completó USB y reportó TX_DONE/count1 bajo la ventana reversible de concesiones Wi-Fi PHY0. La política observada `0x01775774` forzaba bajas las concesiones WLAN; el contraste aplicó `0x01757f5f` y restauró el original.

La captura independiente contiene, con CRC válido, la solicitud propia (paquete396), un ACK dirigido a nuestra MAC (397) y la respuesta de sondeo de Especial (398). El ACK no identifica al transmisor; la respuesta sí identifica al AP. La solicitud coincide con la preservada salvo la duración que actualiza el hardware. Esto confirma emisión y respuesta para este intercambio de gestión.

El ejecutor nativo terminó correctamente. El coordinador encontró JSONL mal formado porque una lectura de registro se intercaló dentro de otro objeto. Se preservó el original y se recuperaron únicamente los dos pares conocidos mediante `tools/recover_grant_log.py`; también se corrigió la emisión del log y se añadió regresión. No se repitió el intento.

PCAP:2727 registros,1709 CRC válidos,1018 inválidos,388 beacons válidos de Especial. Los contadores CCK aumentaron3/3; no se atribuye cada incremento a una solicitud. Contexto CLEAN,170 secuencias disponibles y TX pausado al finalizar. No establece calidad RF general, semántica DPK/TSSI ni asociación.

Evidencia: [observación y hashes](grant-probe-stage102-observation.json).
