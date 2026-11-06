# Etapa124 — auditoría real de DISCOVER broadcast y ARP

2026-11-06. La auditoría offline de122 completó correctamente. Se verificaron260 hashes de entrada, el ejecutable contra el preparado123 y los hashes de stdout/stderr. Los siete registros JSON coinciden con el resumen guardado. Cero acceso USB, H2C o transmisión nueva.

M2/M4 se reprodujeron, M3 se autenticó y el DISCOVER registrado se verificó. Las seis tramas CCMP de grupo pasaron autenticación: DISCOVER propio reflejado y cinco solicitudes ARP (operación1). Cero rechazos, cero pairwise y cero OFFER. Esto ratifica el resultado observado en vivo; la petición broadcast no produjo OFFER dentro de la ventana registrada.

Las cinco solicitudes ARP tienen IPv4 de origen y destino distintas de cero. Ninguna MAC de emisor coincide con la BSSID del AP, ni su MAC de destino ARP coincide con nuestra estación. No se guardaron claves, payload ni direcciones IP descifradas.

## Límites de la clasificación

No se ha demostrado una relación entre estos ARP y nuestro DHCP. La MAC del servidor DHCP o de una interfaz del router puede diferir de la BSSID. Además, una solicitud ARP suele llevar MAC de destino desconocida: no coincidir con nuestra MAC no descarta que pregunte por una IP que el servidor podría ofrecernos. Estos booleanos no permiten atribuir las solicitudes a otro cliente ni excluir comprobaciones de disponibilidad del servidor. No se declara resuelta la causa de la falta de OFFER.

El usuario informó que otro cliente de Especial obtiene IP automáticamente; no equivale a observar cómo responde el servidor a nuestro XID/MAC. Las siguientes incertidumbres son la respuesta del servidor a este cliente y el límite de tres segundos de escucha. Un registro del servidor filtrado por la estación02:00:00:89:22:a1 o una observación más larga permitirían contrastarlas. No hay motivo nuevo para cambiar claves, filtros QoS o descriptores TX basándose en estos ARP.

## Estado conservado

La auditoría no cambia el estado del intento122 ni libera contexto. Último estado físico registrado123: contexto DIRTY, barrera CLEAN,206 secuencias libres, TX pausado y210 registros correctos. No repetir122, borrar reservas o marcar CLEAN manualmente. Cualquier ensayo de radio nuevo requiere recuperación verificada y admisión nueva. Sin OFFER, lease, recepción pairwise física o interfaz del sistema nuevos. Avance ponderado60 %.

Validación de esta etapa: integridad de archivos y coherencia de los resultados reales; no cambia código. El auditor123 ya pasó123 pruebas Python y suites C++.

[Observación y hashes](wpa2-stage124-observation.json), [manifiesto de auditoría](../artifacts/wpa2-offline-stage123/audit.json), [metadatos autenticados](../artifacts/wpa2-offline-stage123/audit.jsonl).
