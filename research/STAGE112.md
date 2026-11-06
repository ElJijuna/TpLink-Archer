# Etapa112 — nuevo intento WPA2 admitido en la época actual

2026-11-05. Preparado, todavía no ejecutado. Admisión real sin USB/TX aprobada sobre la época111 verificada: contexto/fence CLEAN,215 secuencias libres y `.epoch-id` coincidente. Build universal ARM/Intel y suites C++/120 pruebas Python aprobados. Los hashes fijan binario, fuentes, evidencia111 y postflight.

Se conservan todos los intentos anteriores. El marcador nuevo es `37ad-0104-special-channel9-wpa2-recovered-stage112-attempted`; aún no existe. Secuencias802.11 nuevas22–27. Reporte de limpieza1 únicamente en el espacio de reportes de esta época fría: no se interpreta un reporte de una generación anterior como confirmación. La recarga y el drenaje verificados delimitan ese espacio. Al ejecutar se reservarán H2C41–49; aún no se consumieron.

El recorrido mantiene el alcance105: autenticación/asociación, M1→M2→M3 autenticado→M4, un DHCP DISCOVER por CCMP y espera acotada de OFFER autenticada con XID/MAC coincidentes, seguida de desautenticación cuando el estado esté confirmado. Nunca reenvía automáticamente una fase consumida. Si falta confirmación, conserva el marcador/reserva y puede quedar DIRTY. Ventanas de concesión y RX reversibles; captura externa simultánea en9. No se declara seguridad por M3 o por entregar M4 a USB: exige el OFFER coincidente.

No se adquiere lease (sin DHCP REQUEST), no se crea interfaz de red ni se promete conexión utilizable. El intercambio completo, la transmisión BE y los datos cifrados siguen pendientes de prueba física.

Ejecutar una sola vez desde Terminal normal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v
python3 tools/external_probe_capture.py --wpa2-recovered artifacts/wpa2-stage112
```

La contraseña de Especial se introduce solo en el prompt local oculto. No se conserva la del intento anterior. El coordinador la pasa por pipe anónimo, sin archivos, argumentos ni valor de entorno; únicamente el helper de captura usa sudo. Las capturas contienen el tráfico de radio, no claves derivadas ni plaintext recibido. en0 puede desconectarse; compartir la salida, nunca la contraseña.

No ejecutar comandos de etapas105/110 ni borrar sus marcadores. Si la generación o el estado cambia, la admisión rechaza el intento.

Evidencia: [observación112 y hashes](wpa2-stage112-observation.json).
