# Etapa57 — actividad PHY perdida y base recuperada

Fecha: 2026-10-22. Se añadió muestreo RX dentro del contexto reconocido y
un analizador offline que no confunde saturación/reinicio con deltas válidos.
La nueva observación cambia la hipótesis: no había actividad PHY creciente
durante esta ejecución, aunque el camino H2C/C2H continuaba funcionando.

## Instrumentación

Después de los comandos de creación se reinician estadísticas PMAC, y se
leen cada250ms: RMAC RX_ERR_ISR114f4, flags DMA10c00/10c84, RX_STOP8914,
OFDM CCA20e24, CRC CCK/OFDM/HT22e14/20e64/20e60, HCI7880 y CMAC10000.
También se consulta PLE grupo0 y cuota6 mediante el selector DFI restaurable.
Lecturas secuenciales: el snapshot no es atómico ni representa vigilancia
continua. Los flags se leen sin limpiarlos; no se deduce cuándo se activaron.
Fuente de flags: Linux fijado `reg.h:8305-8316`; DFI y contadores mantienen
los límites de las etapas anteriores.

`tools/rx_path_report.py TRACE.jsonl` valida forma/rango/orden temporal,
rechaza snapshots incompletos y devuelve delta desconocido si un contador
CRC OK se satura o retrocede. No estima un contador RMAC que la fuente no
define. Los valores superiores de los contadores CRC no se suman a CRC OK.

## Contexto y contraste sin contexto

Con contexto, agregaciónv3 y gestión completa:21 muestras durante5018ms;
CRC OK CCK/OFDM/HT sin incremento, flags RMAC presentes59hex,
DMA flags0, HCI RX y CMAC RX habilitados en todas las muestras,
RX_STOP host0, PLE grupo0 vacío y cuota6 usada0 en las muestras.
Nueve comandos, ocho ACK de control, retirada reconocida;0 paquetes normales.

59hex corresponde a bits0/3/4/6: CCA timeout, CCA assertion timeout,
DATAON assertion timeout y RX action timeout. Ya estaba presente en la
primera muestra: **no se demuestra que naciera durante esta ventana**.
No son cuatro contadores ni una prueba de cuatro eventos nuevos.

Se repitió la medición sin crear contexto: CCK CCA/CRC y OFDM/HT CRC no
crecieron tras reset. La inactividad no dependía únicamente de mantener
activo el contexto en esa ventana. No se había medido antes de cada comando
de creación; por tanto, todavía no se atribuye el cambio a un comando concreto.

## Recuperación física verificada

Con la reserva en CLEAN, se repitió la secuencia conocida y autorizada:
radio/firmware→PRE/DACK/RXDCK→perfil canal1 y calibraciones TX. Se usaron
los tres binarios conservados de etapa42, sin resetear el ledger de secuencias.
Los tres terminaron0; DACK/RXDCK iniciales limpios y todos los comandos TX
completados. DPK/TSSI mantienen sus semánticas de calidad pendientes.

La medición posterior sin contexto volvió a contar CRC OK:
**CCK349, OFDM49, HT0** en cinco segundos; todavía0 transferencias normales
USB. Scheduler TX quedó0, stderr vacíos. No se volvió a crear contexto tras
esta recuperación: se preserva la base PHY activa para el siguiente contraste.

La retirada ROLE/CAM reconocida por ACK y los campos restaurados no bastan
para garantizar una radio operativa. CLEAN sigue siendo una marca del ciclo
de control, no certificación de estado PHY/RF. No se afirma reversibilidad
completa de los experimentos a partir de sus readbacks de registros.

## Validación y siguiente paso

49 suites C++ y50 tests Python. Cuatro tests nuevos verifican deltas, flags,
ocupación, saturación/retroceso e inputs inválidos del analizador.
Binario exacto del contexto conservado antes de ejecutarse; recuperación
vinculada por hash a los ejecutables conocidos. Evidencia:
[rx-path-stage57-observation.json](rx-path-stage57-observation.json).

Siguiente contraste: medir PHY y controles después de cada comando de
contexto desde esta base recuperada, y contrastar el orden de configuración
de canal/chanctx con el de creación de interfaz de Linux. No continuar
atribuyendo RX0 solo a filtros/USB mientras PHY esté inactiva.
Progreso ponderado40%; conectividad sigue pendiente.
