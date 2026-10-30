# Etapa79 — primera trama con TX_DONE del firmware

2026-10-30. Mac ARM. Avance ponderado **52 %**. No hay conexión Wi-Fi todavía.

## Resultado físico

El paquete corregido se entregó una sola vez por endpoint0x05,96 bytes,
status USB0. El firmware devolvió un C2H TX_REPORT v2 con **tag1, estado0
(TX_DONE), contador1**. El contexto ROLE/CAM se retiró con confirmaciones;
el terminal confirmó TX pausado y los callbacks USB drenaron hasta pending0.
Reserva final CLEAN. Se consumieron9 secuencias:223→214.

Esto acredita finalización TX según el firmware para una solicitud de sondeo
broadcast. No acredita ACK externo ni asociación. La auditoría offline de
las capturas encontró **0 probe responses válidos dirigidos a la MAC de
prueba**. No hay captura independiente por aire ni medición de potencia RF.
No se presenta TX_DONE como una conexión de red utilizable.

El descriptor enviado incluye los defaults WD_PAGE/SW_MLD/DISDATAFB corregidos
en etapa75. El contraste cambia esos campos en conjunto después de reset
eléctrico; no identifica cuál de ellos explica individualmente la diferencia
frente a los reportes fallidos de etapa75.

## Una nueva reserva explícita, sin reintentos

`tools/single_probe_attempt.py` exige el workflow cold-start completo de
etapa78, sus terminales RFK/captura/barrido, ledger idéntico al final de ese
workflow y contexto/fence CLEAN. Archiva el marcador anterior y toma el
bloqueo del proyecto. Crea una reserva durable, de un solo uso, vinculada al
hash de ese historial frío. El ejecutable recibe el FD bloqueado del nuevo
marcador y comprueba inode/estado antes de admitir el ensayo.

La reserva queda conservada incluso si falla el hijo. No se borra el marcador
para permitir reenvíos ni se reutiliza el mismo historial frío. La ejecución
de esta etapa tuvo una entrega USB y el contador declarado de TX fue1;
el software no puede convertir un límite de descriptor en una garantía
universal sobre intentos RF, como mostró etapa75.

```
python3 tools/single_probe_attempt.py artifacts/cold-rfk-workflow-stage78 artifacts/NEW_ATTEMPT
```

Ese historial ya fue consumido. El comando no es repetible automáticamente.
Su validación se refiere a evidencia local controlada de investigación;
no detecta todos los cambios externos posibles del hardware. El emisor
también comprueba físicamente el perfil/canal/potencia world antes de enviar.
La semántica y calidad completas DPK/TSSI conservan sus límites previos.

## Evidencia y validación

[single-probe-stage79-observation.json](single-probe-stage79-observation.json)
registra hashes del binario preservado, paquete y logs. Evidencia principal:
`artifacts/single-probe-stage79/attempt.json`, `probe.jsonl`, `probe.stderr`,
`capture/single-probe.bin` y `archer-single-probe`. El paquete y binario
ensayados están preservados, a diferencia del límite documental de etapa75.

Se añadió después del ensayo validación explícita del log retenido: exactamente
una escritura96/status0, un reporte TX_DONE/count1, cleanup confirmado y
pending0. Esa validación pasó offline y se incorpora a futuras ejecuciones
del coordinador. No se lanzó otro ensayo para probarla.

Auditoría de respuestas: `artifacts/probe-responses-audit-stage79.json`.
Build universal correcto; Intel sin prueba física. `make test` pasa:
81 tests Python y suites C++, con pruebas de permisos de un solo uso,
conservación tras fallo y rechazo de terminales TX incompletos/discordantes.
Log `artifacts/single-probe-tests-stage79-final.log`.

## Progreso y siguiente paso

TX/RX pasa5→7 de15 puntos por descriptor+entrega USB de gestión y reporte
TX_DONE correlacionado en hardware. El total pasa50→52/100; no representa
tiempo restante. Faltan respuesta externa/validación de RF, datos TX/RX,
asociación, seguridad e interfaz macOS, además de Intel/Linux.

El siguiente contraste debe comprobar respuesta de otro equipo o emisión
independiente por aire antes de promover el resultado a enlace Wi-Fi.
