# Etapa144 — retransmisión DISCOVER acotada preparada

2026-11-11. El usuario no tiene acceso al registro DHCP del router. Se preparó una alternativa que no depende de ese acceso: como máximo dos envíos DISCOVER en una misma transacción, con XID constante, secuencias Wi-Fi diferentes y PN CCMP nuevos. No se repite una escritura USB ni se relanza un intento consumido.

El primer presupuesto de espera es aleatorio3–5s. Si no llega OFFER, se drena una vez más la recepción y solo si sigue ausente se usa el único slot de retransmisión. La siguiente espera es aleatoria7–9s; no existe una segunda retransmisión. Si llega OFFER temprano, el slot se omite y se continúa directamente con REQUEST. El campo BOOTP secs se actualiza para retransmisión y REQUEST. El plazo total del ensayo nuevo es30s, con gracia3s solo para desautenticación, dentro de la captura45s. REQUEST/ACK/RELEASE conservan sus validaciones y reportes separados.

Referencia primaria: [RFC2131 sección4.1](https://www.rfc-editor.org/rfc/rfc2131.html#section-4.1), que describe retransmisiones con espera exponencial aleatoria y permite mantener XID. Esto es un experimento limitado a una retransmisión, no un cliente DHCP completo con renovación o reintentos indefinidos. `automatic_retry=false` sigue significando que el coordinador no relanza el experimento; los manifiestos declaran por separado `protocol_discover_retransmissions_max=1` para evitar confundir ambas políticas.

Pruebas: rutas con y sin retransmisión, rechazo de una segunda emisión, secuencia/PN distintos, límites de jitter, secs, REQUEST después de OFFER y ACK/RELEASE con reportes en orden. Suites C++ y125 pruebas Python aprobadas; modos nuevo e histórico compilados ARM/Intel. No se ha probado el cambio por radio ni demostrado la causa de la falta de OFFER.

Estado conservado: contexto DIRTY,206 secuencias libres, marcador141 consumido. No se admite un intento nuevo: los hashes históricos ya no coinciden y falta recuperación verificada y marcador nuevo. Se solicitó retirar el Archer y dejarlo fuera para comprobar ausencia antes de reconectar. No ejecutar141 ni borrar reservas. Cero H2C/TX nuevos en esta etapa; avance físico64 %.

[Observación y hashes](dhcp-stage144-observation.json).
