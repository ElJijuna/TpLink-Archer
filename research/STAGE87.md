# Etapa87 — contexto recuperado sin reset ni nueva transmisión

2026-10-31. Mac ARM. Avance52 %. No se resolvió todavía la falta de respuesta
externa del experimento unicast86.

## Recuperación basada en evidencia

El fallo de86 era un terminal TX conocido (RETRY_LIMIT/count1), seguido de
retirada de contexto confirmada; el error se propagaba antes del commit CLEAN.
Se implementó una finalización específica de esa evidencia, sin borrar a
ciegas el estado DIRTY ni recargar firmware.

`context_cleanup_replay` reconstruye la máquina InterfaceContextLifecycle
con las secuencias41..49 y compara cada uno de los nueve H2C guardados con
los bytes generados por el codec. Recorre las capturas USB por orden y
reproduce los ACK reales, incluido el terminal tag3/state1/count1 antes de
retirar el contexto. Llega al paso9, cleanup_acknowledged, sin H2C pendiente.
El paso5 mantiene su política fuente de solo entrega; no se inventa un ACK.

`tools/finish_probe_cleanup.py` exige observación86, hashes de toda la
evidencia, ledger posterior idéntico y frontera CLEAN. Conserva la reserva
DIRTY anterior, bloquea workflow y contexto, ejecuta el replay y después
una verificación física. Solo compromete CLEAN si ambos terminan y el
ledger/estado inicial siguen iguales.

La verificación física confirmó firmware cargado, identidad6933,2452MHz,
RF9 en sintetizador seleccionadoA y ambos registros digitales, MCC0 y
scheduler0x10398 pausado. El acceso WLAN IO se restaura al salir.

**Resultado: contexto CLEAN, canal9, MCC0,206 secuencias. Cero nuevos H2C,
cero nuevas tramas de red, cero resets de firmware y sin reconectar USB.**
Todos los marcadores de intento permanecen conservados.

## Diagnóstico TX y límites

Se leyeron registros de diagnóstico sin escribir selectores ni activar una
medición:0x1082c,0x108f0,0x108f4,0x20ecc y0x20ed0.
`R_BE_TXCNT` (reg.h:7214) es política/límite de reintentos, no un contador
acumulado de emisiones. IFS-CLM TX/CCA no tenía un intervalo armado/baseline;
sus ceros no prueban ausencia de TX. Los valores se conservan crudos y no
se atribuyen al intento86. No hay evidencia nueva de emisión por aire o ACK.

Para futuros ensayos, el emisor ahora diferencia fallo TX terminal conocido
 de ausencia de reporte/transporte incierto. Tras un terminal conocido y
limpieza confirmada, completa comprobaciones, restaura ventanas y drena USB
antes de CLEAN; después sigue notificando el fallo como error. Un reporte
faltante o limpieza incierta mantiene DIRTY. Esta ruta futura compila, pero
no se hizo otra transmisión para probarla físicamente.

## Validación y evidencia

`make test`:90 tests Python y suites C++; el test con capturas locales cambia
únicamente la identidad del ACK final y comprueba que el replay rechaza la
limpieza incompleta. Otro test verifica que fallar la verificación física
conserva DIRTY y el ledger. El test de capturas se omite en checkouts que no
disponen de estos artefactos locales. Compilan universal los helpers y las
variantes normal, probe1, probe9 y unicast9. Intel sin prueba física.

[cleanup-stage87-observation.json](cleanup-stage87-observation.json) conserva
resultados, hashes, valores crudos y estado final. Evidencia local en
`artifacts/cleanup-finalization-stage87/`. Logs de tests/compilación con
prefijo `artifacts/cleanup-stage87-`.

Siguiente: instrumentación TX con intervalo documentado o captura con
receptor independiente para separar RF, RTS/CTS y recepción de respuesta.
La recuperación no habilita reutilizar los intentos ni confirma asociación.
