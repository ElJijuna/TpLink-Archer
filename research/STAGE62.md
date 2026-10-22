# Etapa62 — auditoría efectiva de entrega RX

Fecha: 2026-10-22. Se comparan24 campos de recepción con la inicialización
RTL8922A CV1 USB2/SCC de la fuente fijada
c03c1a14507dfaf8b856c2608f9ac216021fc222. La auditoría solo lee registros;
no intenta reparar diferencias ni reinicializar DLE/CAM en caliente.

## Alcance de la comparación

`rx_delivery_audit.hpp` conserva dirección, ancho, máscara y valor de fuente.
El ejecutor toma las24 lecturas antes de emitir sus filas JSON, evitando mezclar
las respuestas del bus con una fila incompleta. Captura tres fases: antes de
las ventanas, contexto listo y después de restaurarlas.

| Grupo | Referencia local de fuente |
|---|---|
| Shared CMAC y prerequisitos HCI/USB | `dmac_init.hpp`, `mac_be.c:369-394`, prerequisitos del boot USB |
| Layout PLE y cuotas0/6 | `dle_init.hpp`, tabla USB2 SCC previamente fijada |
| MPDU, FCS, header conversion, forwarding | `mac_be.c:1000-1034`, `dmac_init.hpp` |
| ADDR CAM y filtros | `mac_be.c:1254-1344` |
| RMAC deadlock, PLCP, máximo MPDU, BUSY_CHKSN | `mac_be.c:1527-1572` |
| Colas de reportes | `mac_be.c:1734-1753` |
| Agregación USB del8922A | `usb.c:820-829,848-885` |

La máscara de CUT_AMSDU excluye bit24, cuyo borrado por el silicio ya se
observó en etapa32; se conserva el valor solicitado por la fuente. El máximo
MPDU se compara como parte del valor RX_FLTR_OPT, calculado en etapa33.
No se interpreta forwarding=1 como un destino distinto ni se cambia0x8938.
La rama USB para8922A omite expresamente la configuración RXDMA_SETTING y
endpoint NUMP usada por chips anteriores. No se importan esas escrituras.

## Resultado físico, Mac ARM

| Fase | Campos leídos | Coinciden con inicialización fuente | Diferencias |
|---|---:|---:|---|
| Antes de ventanas |24|22| RXAGG0=0, RXAGG1=40000000hex |
| Contexto listo |24|21| RX_FLTR_OPT=0f170000hex; CTRL/DATA=0 |
| Después de ventanas |24|22| RXAGG0=0, RXAGG1=40000000hex |

Las diferencias de agregación fuera de la ventana corresponden al estado
que la ventana guarda/restaura; durante el contexto RXAGG0=80ff2014hex y
RXAGG1=1fhex coinciden con la fuente. Los tres filtros diferentes durante
el contexto corresponden al perfil de captura: elimina criterios de rechazo,
acepta gestión y mantiene control/datos deshabilitados. MGNT=ffffhex.
Los24 valores finales son idénticos a los24 iniciales.

El resto coincide bajo las máscaras auditadas: Shared CMAC, PLE ready/layout,
cuotas0 y6, APPEND_FCS, CUT_AMSDU con su excepción documentada, conversión
de cabecera, forwarding55hex, interfaz PHY, ADDR CAM, filtro PLCP6fhex,
protección RMAC20f2hex, PLCP length source, IDs de colas de reportes,
DMA USB con RX habilitado y HCI DMA. Esto no certifica el recorrido completo:
solo descarta diferencias visibles en esos campos/momentos/máscaras.

Recepción asíncrona: ocho completados exitosos de36 bytes en0x84, todos ACK;
ningún paquete normal ni beacon. Después se cancelaron y drenaron las nueve
peticiones pendientes. Contexto retirado, TX de red pausado, reserva CLEAN,
salida0, stderr vacío.

Durante5003 ms, contadores PHY CRC OK aumentaron CCK708, OFDM29 y HT0.
PLE grupo0 vacío y cuota6 usada0 en las20 muestras. Flags RMAC59hex,
PHYINFO0ehex, CMAC40hex y dispatcher c0hex persistentes desde la primera
muestra, sin bits nuevos. No se limpiaron ni se pueden fechar eventos repetidos.

## Validación y siguiente contraste

50 suites C++ y55 tests Python pasan. El nuevo test verifica que la auditoría
no escribe, usa anchos correctos y se detiene al primer registro inaccesible
o fallo de transporte. Compilación universal correcta; hardware solo ARM.
La lectura adicional dentro del contexto está protegida por el camino existente
de retiro, de modo que un error de auditoría no omite el intento de cleanup.

No se encontró una discrepancia inesperada que justifique una corrección en
caliente. Sigue pendiente localizar la falta de entrega normal; el próximo
contraste debe revisar la interfaz PHY→MAC y los criterios de aceptación de
tramas frente a la secuencia del chip, sin deducir una causa de flags antiguos.
El avance funcional ponderado permanece en40 %.

Hashes y observaciones: [rx-delivery-stage62-observation.json](rx-delivery-stage62-observation.json).
