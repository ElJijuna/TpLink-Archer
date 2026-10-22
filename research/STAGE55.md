# Etapa55 — ciclo de contexto ejecutado por USB

Fecha: 2026-10-22. Nuevo ejecutor universal `archer-context-rx`:
`make context-rx-universal`, luego
`archer-context-rx --run-owned-source-profile NEW_DIRECTORY`.
Es un perfil diagnóstico del dispositivo propio, no instalación de driver.

## Condiciones y propiedad

Comprueba un único Archer37ad:0104 USB2, chip71, estado firmware/ADDA/DMAC,
canal digital1 de ambas rutas, FCS añadido, scheduler0 y puerto0 inactivo
NO_LINK/sin beacon TX. El formato0.35.41 se basa en el historial controlado
de carga del proyecto; estas lecturas no recuperan una versión independiente
del firmware residente. No debe usarse como autodetección de ABI genérica.

MACID0/CAM0/BSSID_CAM0 se reservan bajo el lock del ledger. La primera
propiedad se apoya en el historial de esta investigación: CAM inicializado
globalmente y ningún comando previo de creación de interfaz. No se inspecciona
un inventario de roles firmware ni se detectan otros clientes externos al lock.
No usar sobre un adaptador con contexto ajeno o estado de origen desconocido.

La reserva persistente `artifacts/37ad-0104-context-reservation` se marca
DIRTY antes de comandos y solo CLEAN tras satisfacer la política de retirada.
Un fallo deja DIRTY e impide repetir automáticamente. Nueve secuencias se
reservan y persisten antes del envío; no se reciclan tras fallo.
MAC local de diagnóstico02:00:00:89:22:a1, BSSID cero, sin asociación.
El estado inicial de pausa MACID **no se midió**; la retirada solicita
despausado/despierto como objetivo explícito de esta prueba. No se afirma
restauración del estado inicial de esos campos.

## Ejecución física reproducida

| Resultado | Primera ejecución | Repetición |
|---|---|---|
| Secuencias reservadas | 67..75 | 76..84 |
| Envíos H2C | 9 | 9 |
| REC ACK pausa/despausa | 2 | 2 |
| DONE0 ROLE/JOIN/CAM/CMAC/retirada | 6 | 6 |
| DMAC | enviado, sin ACK por diseño fuente | igual |
| Transferencias RX USB | 8 | 8 |
| Paquetes Wi-Fi normales | 0 | 0 |
| Beacons interpretados | 0 | 0 |
| Retirada ROLE/CAM | reconocida | reconocida |
| Scheduler de red al terminar | 0 | 0 |

Las ocho transferencias son exclusivamente C2H ACK de12 bytes, no tráfico
de red. Confirman el intercambio H2C→firmware→C2H por bulk USB, incluyendo
correlación de cada comando y secuencia. No prueban entrega de paquetes
normales tipo0 ni contenido/efecto completo de las tablas.
La ruta de control USB funciona; sigue pendiente la ruta de recepción normal.

El ejecutor combina puerto parcial NO_LINK, bitmaps PHY-status, informe PHY
y filtro beacon/probe-response con A_BC limpio. No reproduce todo port_update.
Aplica3s máximos de predrain,5s por ACK, captura5s y límite1024 transferencias.
Mantiene scheduler detenido y motor de respuestas desactivado; no envía
tramas Wi-Fi host ni comandos de asociación. No se deduce silencio RF absoluto.
Los campos de las ventanas se restauraron. Las tablas no se restauran a su
contenido anterior: son recursos propios cuyo rol/CAM se retiró según fuente.

## Fallos y límites

No se reintenta un comando. Timeout/resultado incierto bloquea el ciclo y
retiene DIRTY; no declara una retirada que no confirmó. El operador debe
recuperar firmware y resolver la reserva después de verificar el estado.
Un fallo de captura con contexto todavía conocido intenta la retirada;
un error protocolario que vuelve el contexto incierto no la improvisa.
CLEAN expresa la política del ciclo: ROLE_REMOVE/CAM invalidación DONE0 y
recepción de la solicitud final de pausa; no readback físico de pausa/tablas.

## Verificación

48 suites C++ y46 tests Python; compilación ARM/Intel. Se incorporaron16
fixtures ACK reales de ambas ejecuciones, comprobadas como C2H solamente.
El test nuevo reproduce ambas secuencias completas y mantiene DMAC como
envío sin confirmación. Ambos stderr vacíos, reserva final CLEAN.
Binario exacto de repetición conservado antes de ejecutarlo; el primero no.
Hashes/trazas: [context-usb-stage55-observation.json](context-usb-stage55-observation.json).

Siguiente diagnóstico: recepción normal RMAC/descriptor/despacho con contexto
ahora reconocido. El éxito del ciclo de control no resuelve ni demuestra
calidad RF, RX de red, asociación o conectividad. Avance ponderado40%.
