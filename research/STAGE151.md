# Etapa151 — credencial coincidente y retransmisiones M1 sin respuesta

2026-11-17. La comparación offline150 terminó correctamente: la credencial local autentica M3 de138 y reproduce exactamente M2 de148. El usuario confirmó que la clave no cambió. Se verificaron733 hashes de referencia,344 del intento y hashes del helper/stdout/stderr. No hay evidencia de una credencial distinta en148. La auditoría de referencia también ratifica su OFFER autenticado histórico; no se transmitió ni se reutilizó una contraseña del chat.

## Diferencia observada

La auditoría de metadatos pública de148 encuentra cuatro M1 válidos en transferencias126,169,220 y283: mismo ANonce y replay estrictamente creciente. No encuentra M3 candidato ni desautenticación/disasociación del AP en esa ventana. La referencia138 contiene un M1 y un M3 candidato, cuya autenticación ya está verificada por la auditoría criptográfica.

El descriptor USB de M2 solo difiere en DWORD3:0x36 frente a0x45, las secuencias previstas54/69. Esto no acredita entrega RF. El receptor anterior conservaba el primer M1 y, tras emitir M2, ignoraba los siguientes. Se identifica una ruta faltante del supplicant; todavía no se demuestra que implementarla resuelva físicamente el fallo.

## Respuestas acotadas preparadas

Se añadió generación explícita de hasta tres M2 nuevos ante M1 con el mismo ANonce y replay mayor, sin key data, únicamente antes de aceptar M3. Conserva SNonce/PTK temporal, copia el replay recibido y recalcula MIC. Rechaza duplicados/replay antiguo, ANonce distinto, exceso de presupuesto y cualquier petición posterior a M3/M4. No reinstala claves ni reinicia contadores CCMP.

La fuente primaria de contraste es [wpa_supplicant, procesamiento EAPOL-Key](https://android.googlesource.com/platform/external/wpa_supplicant_8/+/073b9ad1b7e6876417fe0927c6364245ecf26d8b/src/rsn_supp/wpa.c): la respuesta2/4 copia el replay de1/4 y usa el SNonce de la negociación; este se renueva para la siguiente negociación después de3/4. El experimento implementa un subconjunto más restringido, con ANonce fijo y presupuesto tres.

El modo nativo nuevo reserva tres slots opcionales después de M2, cada uno con secuencia Wi-Fi propia. No reutiliza la escritura anterior. M2 no está cifrado por CCMP: estas respuestas no consumen PN CCMP. Si llega M3 sin repetir M1, omite los tres slots. Integra la retransmisión DHCP opcional ya preparada y conserva las barreras de limpieza. Límite total35s, gracia3s de desautenticación, dentro de captura45s.

Pruebas: MIC/replay y límites de respuesta, ANonce distinto, duplicado, prohibición tras M3/M4, no reinstalación; ocho combinaciones de cero–tres respuestas M1 y retransmisión DHCP opcional, secuencias estrictamente crecientes y rechazo de envío duplicado. Suites C++ y126 pruebas Python pasan; modos nuevo e histórico compilan ARM/Intel.

El binario nuevo todavía no tiene admisión de coordinador: faltan recuperación física, identidad nueva, manifiesto y validador del esquema de fases. No ejecutar directamente el binario ni comandos148 anteriores. Contexto DIRTY y206 secuencias libres conservados. Primero retirar el Archer y dejarlo fuera para verificar ausencia, antes de reconectar y recuperar. Efecto por radio pendiente; avance físico64 %, tarea abierta.

[Observación y hashes](wpa2-stage151-observation.json).
