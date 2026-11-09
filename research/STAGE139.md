# Etapa139 — OFFER DHCP cifrado confirmado y limpieza CLEAN

2026-11-08. El intento138 completó las seis fases sin reintentar: autenticación, asociación, M2, M4, DISCOVER cifrado y desautenticación. M3 pasó autenticación criptográfica. Se verificaron el ejecutable conservado, stdout/stderr y PCAP contra los hashes de sus manifiestos; el validador estricto de resultado reprodujo la validación guardada.

## Resultado funcional

Se autenticaron cuatro tramas CCMP de grupo, sin rechazos: DISCOVER propio reflejado, dos ARP y **un OFFER DHCP coincidente**. El parser exige transacción/XID, chaddr, formato IP/UDP/BOOTP, tipo OFFER y opción de servidor válidos. El OFFER de375 bytes llegó tras3361 ms de espera, dentro del presupuesto nuevo de10000 ms y después del límite anterior de3000 ms.

Esto demuestra que en este intento la ventana anterior habría terminado antes de la respuesta. No demuestra que la temporización fuera la única causa de todos los intentos anteriores. El ensayo133 con clave incorrecta no llegó a esta fase. La reflexión del DISCOVER no se contó como OFFER.

La evidencia nativa acredita negociación WPA2 experimental y camino cifrado de DISCOVER/OFFER en ambos sentidos. Las cuatro recepciones son de grupo: **todavía no hay recepción pairwise/unicast física autenticada en este intercambio**. No se envió DHCP REQUEST, recibió ACK de concesión, adquirió lease ni creó interfaz del sistema. Aún no existe conexión IP utilizable.

## Limpieza y estado final

La desautenticación52+5 recibió reporte6, TX_DONE/estado0 y tx_count1. Los ocho pasos ACK de contexto se confirmaron; retirada completada, callbacks drenados. Contexto y barrera de época CLEAN,206 secuencias libres; UUIDda251133-4f2b-4ff7-a08a-8ec21a8ee778. Postflight físico:210 registros coincidentes, TX pausado. El marcador138 permanece consumido: no repetir el comando.

PCAP externo:2656 registros,1368 CRC válidos,1282 inválidos,6 truncados,399 beacons válidos de Especial y14 tramas válidas de nuestra MAC. Estos conteos no bastan para atribuir el OFFER al monitor externo: su prueba criptográfica es nativa. La captura terminó con `capture_exit=0`; la salida1 del helper corresponde al fallo de restauración del canal original de en0. Reconectar Wi-Fi del Mac si no se restauró automáticamente.

## Avance y siguiente frontera

Avance ponderado64/100: datos13/15 y asociación/seguridad11/15. Es una estimación de alcance, no tiempo restante. Quedan recepción unicast, DHCP REQUEST/ACK y ciclo de lease, integración de interfaz macOS, reconexión/renovación de claves, calidad RF completa e Intel/Linux físicos. El siguiente paso debe preparar y validar la continuación DHCP y su limpieza antes de otro envío; no reutilizar138.

No cambió código ni hubo nuevas transmisiones en139. La preparación138 pasó124 pruebas Python, suites C++ y compilación ARM/Intel; aquí se verificaron la ejecución real, integridad y postflight.

[Observación y hashes](wpa2-stage139-observation.json).
