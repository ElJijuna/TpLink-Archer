# Etapa54 — coordinador offline del contexto estación

Fecha: 2026-10-22. `InterfaceContextLifecycle` compone los codificadores de
etapas51-53 siguiendo el orden de `rtw89_mac_vif_init` y `vif_deinit` de la
fuente fijada `c03c1a14507dfaf8b856c2608f9ac216021fc222`.
No contiene transporte, esperas de reloj, acceso a hardware ni liberación
automática de recursos. El ejecutor aún pendiente debe aportar esas operaciones.

## Contrato y orden

El caller declara propiedad exclusiva de MACID/CAM y scheduler detenido,
aporta nueve secuencias distintas ya reservadas y el estado inicial de pausa
MACID. La clase valida campos de todos los frames antes de emitir uno.
No reserva recursos por sí misma ni deduce propiedad de índices en firmware.

| Paso | Comando | Condición de avance |
|---|---|---|
| 0 | MACID despausa/salida de sueño | ACK recepción |
| 1 | ROLE CREATE | DONE0 |
| 2 | JOIN NO_LINK/dis_conn | DONE0 |
| 3 | CAM CREATE V0 | DONE0 |
| 4 | CMAC G7 | DONE0 |
| 5 | DMAC V2 | envío confirmado por transporte, sin ACK |
| 6 | ROLE REMOVE | DONE0 |
| 7 | CAM invalidación | DONE0 |
| 8 | pausa/sueño MACID inicial | ACK recepción |

Tras paso5 se detiene en `ready` hasta solicitar explícitamente retirada.
`ready` significa que terminó esta secuencia con sus políticas respectivas,
no que DMAC esté leído/verificado, que la interfaz esté asociada o que RX funcione.
Tras paso8 usa `cleanup_acknowledged`: ROLE/CAM confirmaron las órdenes de
retirada y firmware recibió la solicitud de pausa inicial. El ACK de recepción
no prueba que esa pausa haya terminado de aplicarse; falta readback runtime.
Restaurar pausa inicial es una decisión diagnóstica adicional al deinit fuente.

## Resultados inciertos

`issue()` marca el comando emitido antes de entregarlo al transporte y no
permite repetirlo. `sent_successfully()` distingue envío de ACK; un ACK REC
no sustituye DONE en los comandos que lo requieren. ACK de otro comando o
secuencia se ignora. Se conserva el historial para detectar errores tardíos
de comandos ya reconocidos. Duplicados idénticos no avanzan dos pasos.

Fallo de transporte/timeout debe notificarlo el ejecutor mediante
`mark_transport_uncertain()`. Retorno firmware no-cero, ACK malformado o
confirmaciones contradictorias también producen estado `uncertain`.
No se permite continuar ni iniciar retirada normal desde ese estado.
Es una política conservadora del diagnóstico, no una limitación exigida
por Linux: evita declarar recursos libres o reenviar una creación ambigua.
El ejecutor deberá recuperar/resetear firmware de forma explícita si ocurre.

## Validación y pendiente

47 suites C++ y46 tests Python; ASan/UBSan y compilación universal ARM/Intel.
El test nuevo simula los nueve pasos, fallos en cada posición, bloqueo de
reenvío, ACK ajeno, REC insuficiente para DONE, error tardío, payload truncado,
secuencias repetidas, guardas de recursos y restauración solicitada de pausa
inicial tanto activa como inactiva.
Registro/hashes: [context-lifecycle-stage54.json](context-lifecycle-stage54.json).

No hubo acceso USB ni se consumieron secuencias reales. Próximo trabajo:
ejecutor nativo con propiedad de recursos comprobable, reserva de secuencias,
predrain, plazos acotados, registro de cada envío/ACK y captura RX entre
creación y retirada. El perfil de puerto sigue siendo diagnóstico parcial;
no se presenta como implementación completa de `mac_port_update`.
Progreso ponderado40%; último RX físico sigue0 en etapa51b.
