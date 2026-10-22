# Etapa56 — post-inicialización USB y filtro completo de gestión

Fecha: 2026-10-22. Se identificó un paso de USB todavía ausente del perfil:
`usb.c:820-829,851-883` de Linux fijado
`c03c1a14507dfaf8b856c2608f9ac216021fc222` configura agregación RXv3 en
RTL8922A durante `mac_post_init`. Definiciones `usb.h:32-38`.

`with_usb_rx_aggregation` aplica temporalmente RXAGG0=80ff2014
(enable1, número255, tiempo32 unidades de32us, buffer20KiB) y RXAGG1=1f.
Conserva los valores originales y los restaura con readback incluso si falla
el callback/escritura. Requiere chip71, scheduler0 y HCI habilitado.
No reinicia USB, HCI o firmware. La política no modifica el perfil base.

## Dos contrastes físicos ARM

| Perfil | Etapa56 | Etapa56b |
|---|---|---|
| Agregación RXv3 | activa temporalmente | igual |
| Filtro gestión | beacon/probe-response120 | todos los subtiposffff |
| Filtros control/datos | 0/0 | 0/0 |
| Secuencias | 85..93 | 94..102 |
| H2C enviados / ACK | 9/8 | 9/8 |
| Transferencias RX de control | 8 | 8 |
| Paquetes normales / beacons | 0/0 | 0/0 |
| Retirada de contexto | reconocida | reconocida |

El filtroffff es el valor aceptado por `rx_fltr_init_be`,
`mac_be.c:1280-1331`; se contrastó sin abrir datos/control ni habilitar sniffer.
Ambos perfiles mantienen A_BC limpio, motor de respuestas desactivado y
scheduler TX de red detenido. No se enviaron tramas Wi-Fi de asociación.

Readbacks en ambas pruebas:

| Registro | Antes | Durante | Restaurado |
|---|---|---|---|
| RXAGG0 6000 | 0 | 80ff2014 | 0 |
| RXAGG1 6004 | 40000000 | 1f | 40000000 |

No se asigna significado al bit30 inicial de RXAGG1; se conserva y restaura.
Los dos binarios exactos se guardaron antes de ejecutarse. Stderr vacío,
reserva de contexto final CLEAN según la política y límites de etapa55.

## Conclusión y límites

La post-inicialización USB faltaba en la reconstrucción, pero configurarla
en esta ventana no bastó para recibir paquetes normales. Abrir todos los
subtipos de gestión tampoco cambió el resultado. Esto no demuestra que la
agregación sea innecesaria ni que otros filtros/estados RMAC estén correctos.

El despacho normal en `mac_be.c:1024-1029` escribe el valor1 en cuatro
campos de DISP_FWD_WLAN_0, igual que el plan ya aplicado (55hex). No se
reinterpretó ni cambió ese valor a partir del nombre del campo: el camino
normal y C2H pueden tener procesamiento distinto aunque compartan USB.

49 suites C++ y46 tests Python; compilación del ejecutor ARM/Intel.
La suite nueva prueba valores fuente, restauración tras cada fallo y guardas
sin escrituras; pasó ASan/UBSan. La prueba de ventana RX incluye gestiónffff
y verifica que control/datos siguen bloqueados.
Evidencia/hashes: [usb-rx-stage56-observation.json](usb-rx-stage56-observation.json).

Pendiente aislar la entrega RMAC/descriptor y despacho normal dentro del
contexto reconocido, con observación de estado/contadores específicos.
No hay aún tramas de red entregadas al host. Avance ponderado40%.
