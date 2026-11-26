# Etapa177 — preparación de ARP pairwise por etapas

2026-11-25. El usuario pidió continuar etapa por etapa. Se implementó solo la primera comprobación unicast: resolver la puerta de enlace del ACK DHCP y exigir una respuesta ARP unicast autenticada con clave pairwise. No se añade ICMP ni interfaz de macOS.

GatewayArpProbe valida lease no nulo, máscara contigua y puerta de enlace unicast dentro de la misma subred, evitando host/red/broadcast. Emite una solicitud ARP (sin retry) con IP del lease y MAC de la estación; la respuesta debe ser op2 Ethernet/IPv4, destino y targetIP de la estación, senderIP del gateway y senderMAC igual al origen Ethernet/Addr3. Solo se admite tras descifrado CCMP y recepción pairwise. Padding permitido únicamente cero. Formato basado en [RFC826](https://www.rfc-editor.org/rfc/rfc826.html).

Se verifica localmente que volver a descifrar el mismo frame recién autenticado falla exactamente por replay. No se retransmite el frame por radio para esta comprobación. El resultado no demuestra por sí solo la reacción del AP a un replay transmitido ni una prueba completa de todos los TID.

Después del ACK y antes de RELEASE, la solicitud ARP pide TX_DONE/count1 con tag15 y espera hasta3s por respuesta. Si tiene TX_DONE pero no respuesta, o faltan metadatos gateway, intenta RELEASE y desautenticación antes de informar el fallo. Si la entrega ARP queda incierta por falta de reporte, no envía otra fase que oculte esa incertidumbre: conserva DIRTY. Un resultado exitoso requiere ARP/reporte/respuesta/replay y cierre; no se acepta una fase ARP duplicada. El estado de liberación del servidor sigue sin confirmación.

Layoutv3: M2=2, M4=6, DISCOVER=7, REQUEST=9, retryREQUEST=10, ARP=11, RELEASE=12, deauth=13. Secuencias Wi-Fi150–163, distintas de174. ACQ habilitado; tags12/11/10/15/13/14. Auditor actualizado para conservar layouts históricos y verificar solicitud ARP cifrada y respuesta pairwise contra el lease guardado.

Pasaron138 pruebas Python y suites C++, incluyendo ARP malformado/ajeno/grupo, gateway fuera de subred, cifrado/tampering/replay reales con credencial pública dummy, slots opcionales y duplicados de envío. Binario y auditor ARM/Intel compilados. Sin efecto físico nuevo comprobado todavía. El Archer reapareció como instalador; se verificó frío y se recuperó por separado en178.

[Preparación y hashes](unicast-stage177-observation.json).
