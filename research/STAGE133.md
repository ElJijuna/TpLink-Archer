# Etapa133 — escucha ampliada ligada a la recuperación actual

2026-11-08. Sustituye la preparación130, nunca ejecutada, tras detectar en131 que el dispositivo volvió al modo instalador. Vinculado a recuperación132, UUID567b917d-aff8-456e-8644-813ec6e9c913, contexto/fence CLEAN y215 secuencias libres.

Mantiene la hipótesis de130: un único DISCOVER broadcast, sin retransmisiones, escucha DHCP de hasta10 segundos recortada por plazo global20s y gracia exclusiva de limpieza3s. OFFER autenticado y coincidente sigue siendo el criterio de completar DHCP; una reflexión no libera reservas. El efecto de ampliar la escucha todavía no está comprobado por radio.

Secuencias Wi-Fi46–51, H2C41–49, reporte de limpieza5 y marcador nuevo `37ad-0104-special-channel9-wpa2-recovered-stage133-attempted`, todavía ausente. Marcadores históricos116/122 intactos. Compilación ARM/Intel, suites C++ y123 pruebas Python aprobadas. Admisión real verificada sin prompt, reserva nueva, USB o TX.

Ejecutar una sola vez como usuario normal en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --wpa2-recovered artifacts/wpa2-stage133
```

La contraseña de Especial se introduce únicamente en el prompt local oculto. Captura simultánea, asociación/WPA2 y un DISCOVER cifrado; no adquiere lease ni crea interfaz de red. Puede desconectar en0. Compartir salida completa sin contraseña; no hay reintentos automáticos y un fallo requiere análisis. No ejecutar comandos130/122 antiguos.

[Preparación y hashes](wpa2-stage133-observation.json). Avance ponderado conservado en60 % hasta obtener evidencia nueva.
