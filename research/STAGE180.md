# Etapa180 — ARP pairwise verificado y cierre CLEAN

2026-11-25. El intento179 completó WPA2, lease DHCP, una solicitud ARP al gateway y una respuesta ARP pairwise autenticada con identidades coincidentes en14ms. Volver a descifrar localmente ese mismo frame fue rechazado por replay; no se transmitió un replay por radio. Resumen nativo: nueve tramas autenticadas, ocho de grupo, una pairwise y cero rechazos del flujo normal.

Se reprodujeron desde USB los cinco reportes TX_DONE/count1: DISCOVER12, REQUEST11, ARP15, RELEASE13 y deauth14. RELEASE, desautenticación, ocho ACK de retirada de contexto y drenaje completaron. No se usaron respuestas M1 ni retransmisiones REQUEST. Hashes de helper, stdout, stderr y PCAP verificados y validador unicast repetido independientemente. Postflight210 registros correctos, TX pausado, contexto/fence CLEAN y206 secuencias libres en épocaabec701b-e225-4e2c-a716-8dc1ee4dc5c6.

Monitor exit1 por restauración de Wi-Fi del Mac (kA11NotSupportedErr); tcpdump guardó2640 paquetes y reportó cero drops de kernel. Reconnectar el Wi-Fi del Mac si no regresó automáticamente. PCAP SHA256152d38abb7859455dd4a95c766c22a1a0ac9fcbb28ab0fa95aca10cfd5f54783. La restauración fallida de en0 no es un fallo del ensayo del Archer.

La recepción pairwise ARP queda demostrada para este perfil; no demuestra todavía un intercambio IP, todos los TID/tamaños, rekey ni rechazo de replay por el AP. RELEASE/TX_DONE no confirma procesamiento del servidor. No se creó interfaz de red ni se configuró IP en macOS. No repetir179 ni desconectar el adaptador para este análisis; el siguiente ensayo puede admitirse solo si el estado físico y la época siguen coincidiendo.

[Evidencia y hashes](unicast-stage180-observation.json).
