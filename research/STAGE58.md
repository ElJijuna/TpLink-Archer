# Etapa58 — PHY medida por transición

Fecha: 2026-10-22. Desde la base recuperada de etapa57 se midieron16
ventanas de250ms: antes de configuraciones, después de agregación, puerto,
PHY-status, informe PHY, filtros, cada uno de los nueve comandos y tras
restaurar ventanas. Cada medición reinicia estadísticas PMAC; los valores
no son acumulativos entre fases y no equivalen a tasas precisas.

Se conservaron CRC CCK/OFDM/HT, FEN, reset BB, RXCCA, PD_CTRL, concesiones
PMAC, selección MAC, flags RMAC y función CMAC. También se consultó RF0 de
ambas rutas: valores instantáneos, no prueba de permanencia en un modo.
Las máscaras PD/reset/RXCCA provienen del `reg.h` fijado de etapas anteriores.
No se escribieron esos controles: solo el reset estadístico ya documentado.

**No se reprodujo la parada PHY.** Hubo CRC correctos CCK después de cada
paso, con valores9..28 por ventana. OFDM también creció en todas las fases
salvo la última ventana, donde CCK sí creció. PD_CTRL permaneció2840e1bf.
La recepción PHY siguió activa durante el contexto y después de retirarlo.

Por tanto, la inactividad observada en etapa57 no se atribuye a un comando
concreto ni se considera consecuencia inevitable del contexto. La recarga
previa recuperó actividad, pero estas observaciones no aíslan la causa de
la parada anterior. Mantener ambas evidencias evita convertir una hipótesis
temporal en una regresión reproducida que no se ha demostrado.

Se mantuvo la captura de cinco segundos con muestreo RMAC/PLE de etapa57.
El ciclo de nueve comandos terminó con retirada reconocida y scheduler0;
continúa sin paquetes Wi-Fi normales por USB. La base PHY activa al salir
permite volver a contrastar entrega RMAC/DMA/dispatcher con recepción real
de radio durante el contexto, no solo apoyándose en una medición histórica.

49 suites C++ y50 tests Python; ejecutor compila ARM/Intel, binario exacto
conservado antes de ejecutarse. El muestreo por transición es instrumentación
nativa; no se presenta como cobertura de fallos adicional al coordinador.
Valores completos, resumen de la ventana y hashes en
[context-phy-stage58-observation.json](context-phy-stage58-observation.json).
Progreso ponderado40%; no hay conectividad Wi-Fi.
