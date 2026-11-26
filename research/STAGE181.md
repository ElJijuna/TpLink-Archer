# Etapa181 — preparación de un único ICMP al gateway

2026-11-25. Después de demostrar ARP pairwise en179, se implementó el siguiente paso separado: un ICMP Echo a la puerta de enlace ya resuelta por ARP, usando únicamente el lease temporal. No envía tráfico a Internet ni configura IP/rutas/interfaz macOS.

GatewayIcmpProbe genera un solo Echo de52 bytes MSDU con identificador y cookie16 aleatorios en el llamador. La respuesta debe ser pairwise autenticada, provenir de la MAC resuelta y la IP del gateway, dirigirse a la IP del lease y devolver identificador/secuencia/cookie con checksums IPv4/ICMP válidos. Rechaza fragmentación, longitudes inconsistentes y padding no cero. Se comprueba localmente replay del frame autenticado sin emitir otro paquete. Formato basado en [RFC792](https://www.rfc-editor.org/rfc/rfc792.html).

El envío pide TX_DONE tag9 y espera hasta3s por respuesta. No repite ARP ni Echo. Si el envío es terminal pero falta una respuesta válida, intenta RELEASE y el cierre antes de informar el error. Si falta el reporte y la entrega queda incierta, conserva DIRTY. Un timeout no permite por sí solo distinguir filtrado/ausencia de respuesta del gateway de pérdida anterior a la captura.

Layoutv4 añade Echo12 antes de RELEASE13 y deauth14; secuencias Wi-Fi200–214, distintas de179. El auditor conserva layouts históricos y verifica Echo cifrado y respuesta pairwise contra el lease, ARP resuelto e identificador/cookie guardados cifrados. Sin claves ni plaintext persistidos.

Pasaron142 pruebas Python, suites C++ y compilación ARM/Intel de helpers. Incluyen checksums/identidades/cookie, fragmentación, slots y duplicados, continuidad CLEAN con ledger/época exactos y fixture cifrado de Echo/reply con corrupción. No hay efecto físico ICMP comprobado todavía. La admisión futura utiliza el cierre verificado180, sin forzar recarga cuando CLEAN y época siguen intactos.

[Preparación y hashes](icmp-stage181-observation.json).
