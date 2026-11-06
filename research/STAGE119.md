# Etapa119 — observación nativa de datos autenticados

2026-11-06. La auditoría118 demostró que el receptor podía recibir tráfico útil sin completar DHCP. El callback nativo anterior descifraba las tramas y solo conservaba el resultado del parser OFFER, por lo que no informaba de DISCOVER reflejado ni ARP autenticado.

Se añadió una observación de metadatos en la misma ruta de descifrado: longitud, EtherType SNAP, grupo/unicast, coincidencia exacta de DISCOVER con origen autenticado propio y OFFER estricto. No imprime claves ni payload. Un resumen acumula autenticadas, rechazadas, grupo, pairwise, reflexiones y ofertas; se emite también tras timeout de fase, antes de intentar limpieza. Las tramas ajenas, de gestión o no protegidas se ignoran antes de contabilizar rechazos CCMP.

La reflexión no completa la espera DHCP, no libera reservas y no altera reintentos, paquetes, tiempos ni secuencias. Se conserva el marcador116 consumido; el binario compilado no representa un intento nuevo admitido. El auditor histórico117 y su evidencia permanecen intactos.

## Validación

La prueba C++ utiliza la misma función que el callback y tramas cifradas con credenciales públicas ficticias. Comprueba DISCOVER reflejado sin OFFER, origen diferente, XID diferente, OFFER pairwise, replay y corrupción con FCS recalculada. Tras el fallo de autenticación acepta la trama original con el mismo PN: el fallo no avanza el contador de replay. La prueba pasa en los perfiles base y recuperado. Suites C++ y122 pruebas Python aprobadas; compilación universal ARM/Intel correcta. No se ha probado esta instrumentación nueva por radio.

## Estado físico y continuación

La inspección actual devuelve `cold_profile=false`: BB control2160263175, BB reset2147057657, firmware control201326626. Solo se leyeron registros mediante la ventana de acceso restaurada; cero H2C, reset de firmware o transmisión nueva. No se ejecutó recuperación ni se liberó el contexto DIRTY.

Para avanzar hace falta desconectar físicamente el Archer, esperar cinco segundos y reconectarlo. Primero debe comprobarse la enumeración y, si reaparece el instalador, recuperar la interfaz Wi-Fi; después se verificará el perfil frío antes de recargar firmware. No repetir el comando116 ni borrar marcadores o reservas. Un nuevo ensayo requiere otra época y admisión con marcador nuevo.

La causa de la ausencia de OFFER sigue abierta. Esta instrumentación permite separar progreso CCMP de éxito DHCP en el próximo ensayo, sin atribuir aún el fallo al AP, al tiempo de espera o a recepción unicast. Avance ponderado conservado en60 %.

Evidencia y hashes: [observación119](wpa2-stage119-observation.json).
