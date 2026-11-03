# Etapa100 — tag7 detenido antes de TX y drenaje de entrada corregido

2026-11-03. Avance ponderado52%; todavía sin conexión Wi-Fi utilizable.

El usuario ejecutó el coordinador de etapa99 en Terminal. La captura externa
terminó correctamente: capture_exit0,4801 paquetes,0 descartes del kernel.
La restauración exacta del canal100 y la alternativa20MHz fallaron−3903;
monitor_exit1 corresponde a esa restauración, no a tcpdump.

El ejecutor tag7 se detuvo en el drenaje anterior al contexto: no hay
firmware_bulk_write ni single_probe_usb_write en su registro completo.
No existe medición PHY antes/después de una transmisión. El marcador tag7
permanece RECOVERY_RESERVED y las nueve secuencias59–67 reservadas se
conservan consumidas, aunque no se enviaron comandos. Contexto CLEAN por
la guarda de reserva sin envío; ledger68 reservadas/188 libres. No se
reutiliza ni reintenta tag7.

CRC32 externo:4069 válidos,727 inválidos,5 truncados;4801 paquetes a2452MHz,
395 beacons válidos de Especial,590 ACK,506 CTS y0 tramas válidas con nuestra
MAC. Como no hubo envío Archer, esto es una captura pasiva y no un resultado
negativo nuevo de transmisión RF.

El registro contiene879 callbacks USB exitosos y242 transfers persistidos:
la cola creció durante el drenaje. El bucle anterior espera hasta10ms en el
endpoint vacío después de cada paquete del otro. Se añadió sondeo sin espera
para el transporte asíncrono y se vacían las colas disponibles antes de
esperar de nuevo. Se mantiene el plazo3s y el límite1024; el transporte
síncrono conserva su comportamiento anterior. Un caso de regresión reproduce
el fracaso con cola finita y valida ambos órdenes de endpoints y un flujo
continuo que debe agotar el plazo.

Prueba física acotada del algoritmo corregido:40 transfers,quiet=true,
0 H2C/0 tramas nuevas, filtros restaurados, callbacks abortados/drenados.
Verificación posterior: firmware listo,canal9,MCC0,TX pausado. El ledger
sigue en188 libres. La prueba no reproduce el backlog original completo;
valida el drenaje actual, no garantiza una futura captura sin fallos.

106 tests Python y suites C++ aprobados; tag8 compila universal Intel/ARM.
No se probó Intel físicamente. REA volvió a reconocer el mismo driver Windows,
pero Hopper no respondió a la comprobación del lease: no se obtuvo nueva
descompilación y se cerró el target sin modificar la sesión propietaria.
No hay nueva interpretación confirmada de DPK path4 ni éxito global TSSI.

Tag8/secuencia8 está preparado con marcador distinto y admisión ligada por
hash a los registros íntegros del tag7 y al drenaje pasivo, además de96/98.
La trama conserva104 bytes/CCK1/noRTS salvo tag y secuencia. El coordinador
sigue exigiendo Especial a2452MHz en la captura viva antes del envío.

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v
python3 tools/external_probe_capture.py --phy-drained artifacts/phy-probe-stage100
```

Estado de los tres objetivos:1) captura externa ejecutada, pero medición PHY
con TX pendiente;2) corregido un bloqueo de drenaje previo al envío, todavía
sin localizar/corregir el fallo RF original;3) sin respuesta dirigida del AP,
asociación, seguridad ni datos. Preparar tag8 no completa ninguno de ellos.

Evidencia: [input-drain-stage100-observation.json](input-drain-stage100-observation.json).
