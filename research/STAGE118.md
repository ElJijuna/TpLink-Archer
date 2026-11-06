# Etapa118 — ida y vuelta CCMP de grupo autenticada

2026-11-06. La auditoría local del intento116 terminó correctamente, sin acceder al USB ni transmitir. Se verificaron los253 hashes de entradas contra la captura original, el hash del ejecutable contra el preparado en117 y los hashes de stdout/stderr. Los cinco registros JSON coinciden con el resumen guardado; stderr vacío. No se necesitó volver a solicitar la contraseña para verificar estos metadatos.

## Resultado

M2 y M4 se reprodujeron exactamente; M3 pasó autenticación y el DISCOVER cifrado registrado se validó. Las cuatro tramas protegidas del AP pasaron autenticación CCMP: una carga IPv4/UDP68→67 de336 bytes y tres ARP de36 bytes. Todas son de grupo; ninguna fue rechazada.

La primera es DHCP DISCOVER (tipo1), con XID y chaddr coincidentes. Además, su carga completa coincide con nuestro DISCOVER registrado y su Addr3 autenticada corresponde a nuestra estación. Esto acredita que el AP recibió y reenvió el paquete propio enviado cifrado, y que el adaptador recibió su copia cifrada de grupo. Es evidencia de un camino limitado de datos cifrados de ida y vuelta en la captura nativa. El monitor independiente no corroboró estas tramas de datos.

**No se encontró DHCP OFFER.** DISCOVER reflejado no equivale a oferta ni a dirección asignada. Tampoco hay recepción pairwise/unicast autenticada en esta captura, lease, interfaz del sistema ni conexión IP utilizable. El resultado no determina por qué falta OFFER ni valida calidad RF completa, renovación de claves o reconexión.

## Estado y siguiente frontera

La auditoría no consume secuencias ni libera reservas. Se conserva el último estado registrado: contexto DIRTY y206 secuencias libres, con TX pausado según el postflight117. No se repite116 ni se modifica su resultado histórico de espera DHCP fallida. Cualquier nuevo ensayo de radio requiere resolver primero la frontera de recuperación; estos resultados offline no autorizan marcar el contexto CLEAN.

La siguiente comprobación funcional debe distinguir recepción unicast cifrada y respuesta DHCP de la reflexión de broadcast ya demostrada. Esta etapa documenta evidencia; no cambia código de transmisión ni inicia otro ensayo. La validación anterior del auditor incluye122 pruebas Python y suites C++; aquí se comprobaron integridad y coherencia de la ejecución real.

El avance ponderado pasa de56 a60: datos11/15 y asociación/seguridad9/15. Es una estimación de alcance, no de tiempo restante ni de disponibilidad de una conexión utilizable.

Evidencia: [observación118 y hashes](wpa2-stage118-observation.json), [manifiesto de auditoría](../artifacts/wpa2-offline-stage117/audit.json), [salida de metadatos](../artifacts/wpa2-offline-stage117/audit.jsonl). La etapa117 conserva lo conocido antes de ejecutar esta auditoría.
