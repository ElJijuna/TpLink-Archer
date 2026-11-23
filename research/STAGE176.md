# Etapa176 — ciclo WPA2/DHCP con cierre verificado

2026-11-24. El intento174 se ejecutó en artifacts/supplicant-stage174-capture; la ruta original174 vacía se conserva. Terminó con exit0 y validación nativa completa. Monitor exit0 y PCAP con SHA25680f602e2c74f51491af2c162b1f773d0015d398be611a116f020b1415d91cfca. Se verificaron hashes de helper, stdout, stderr y captura, además de repetir independientemente el validador del resultado.

Autenticación/asociación y WPA2 completados. DISCOVER recibió TX_DONE12/count1 y OFFER autenticado en24ms. REQUEST recibió TX_DONE11/count1 y ACK DHCP autenticado en12ms. Se reconoció un lease temporal. RELEASE obtuvo TX_DONE13/count1 en18ms y desautenticación TX_DONE14/count1 en12ms. El reporte RELEASE confirma el resultado MAC del envío; no confirma que el servidor procesara RELEASE ni que eliminara el lease.

Los cuatro reportes se reprodujeron desde las transferencias guardadas: rx175(tag12),178(tag11),181(tag13),182(tag14), todos estado0/count1. La retirada de contexto obtuvo ocho ACK, USB drenado y scheduler pausado. Postflight210 registros correctos, contexto/fence CLEAN y206 secuencias libres en épocaf99dbe27-5d10-4876-bb30-aa835c07f33e. No se enviaron paquetes adicionales durante el análisis.

No se usaron respuestas M1 adicionales, retransmisión DISCOVER ni retransmisión REQUEST; estas ramas siguen sin ejercicio físico en este ensayo. El resultado confirma el efecto de ACQ sobre los reportes de DISCOVER, REQUEST y RELEASE para este perfil. Cuatro tramas de datos autenticadas, todas de grupo; cero rechazos y cero pairwise. No se creó interfaz ni se configuró IP/rutas en macOS.

Avance ponderado conservador65/100 (ruta de datos14/15). No es porcentaje de tiempo restante. El ciclo experimental WPA2/DHCP y cierre quedó verificado, pero la tarea de conexión utilizable permanece incompleta: recepción unicast cifrada, integración de red macOS, ciclo de claves/reconexión y validaciones RF/plataforma pendientes. No desconectar ni repetir174 para este análisis; preservar el estado CLEAN y las reservas. No hay otro comando de radio publicado.

[Evidencia y hashes](lease-stage176-observation.json).
