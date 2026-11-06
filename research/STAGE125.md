# Etapa125 — escucha DHCP ampliada, pendiente de recuperación

2026-11-06. Tras confirmar en122/124 DISCOVER broadcast aceptado y reflejado sin OFFER en tres segundos, se preparó una ventana DHCP de hasta diez segundos. Es un contraste de temporización, sin cambiar DISCOVER, claves, filtros o añadir retransmisiones. Todavía no se ha probado por radio.

La espera conserva un límite global de veinte segundos desde el inicio nativo; solo la limpieza puede usar tres segundos adicionales. Si queda menos tiempo global, la ventana DHCP se acorta. Las demás respuestas mantienen presupuesto de tres segundos. Cada espera registra presupuesto solicitado, duración real, si es limpieza y si recibió respuesta, sin payload ni secretos. OFFER sigue siendo el único criterio que completa la fase DHCP; una reflexión no libera contexto.

Pruebas deterministas sobre la misma función de plazos usada por el ensayo verifican diez segundos disponibles, recorte al límite global, plazo ya vencido y gracia exclusiva de limpieza; no requieren dormir. Los perfiles base y recuperado se prueban por separado: el base conserva espera DHCP de tres segundos. Suites C++ y123 pruebas Python pasan; binario universal ARM/Intel compilado.

La inspección física actual devuelve `cold_profile=false`, sin H2C ni reset. Se conservan contexto DIRTY,206 secuencias libres y marcador122 consumido. No hay una admisión nueva: el binario actual sigue ligado al marcador consumido122 y los hashes históricos impiden volver a admitirlo. No ejecutar el comando122 ni borrar reservas.

Para continuar hace falta desconectar físicamente el Archer, esperar cinco segundos y reconectarlo. Después se verificará enumeración/perfil frío, recuperación y nuevo contexto antes de asignar otro marcador y secuencias. No se solicita todavía ejecutar WPA2. La evidencia nueva aquí es validación de código y estado, no un OFFER ni avance de conectividad. Estimación conservada en60 %.

[Observación y hashes](wpa2-stage125-observation.json).
