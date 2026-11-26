# Etapa179 — primera comprobación unicast: ARP del gateway

2026-11-25. Ensayo nuevo ligado a recuperación178, épocaabec701b-e225-4e2c-a716-8dc1ee4dc5c6, contexto/fence CLEAN y215 secuencias libres. Marcador179 nuevo;174 consumido y conservado. Admisión actual aprobada sin USB, credenciales ni TX.138 pruebas Python, suites C++ y compilación ARM/Intel aprobadas.

Obtiene lease temporal mediante el protocolo WPA2/DHCP validado. A continuación envía una sola solicitud ARP para la puerta de enlace proporcionada por el ACK, dentro de la subred del lease. Exige respuesta pairwise CCMP con campos ARP/origen/destino coincidentes, TX_DONE del envío y rechazo local de replay del mismo frame autenticado. Espera ARP hasta3s; no retransmite ARP. No hace ping, no adquiere/configura interfaz de red ni IP en macOS.

Intenta RELEASE/desautenticación y retiro de contexto al terminar si los envíos son terminales conocidos, también cuando falta la respuesta ARP. Ante entrega incierta conserva DIRTY y no reintenta. RELEASE no confirma procesamiento por el servidor. Mantiene los límites previos de respuestas M1 y una retransmisión DISCOVER/REQUEST; no relanza automáticamente el ensayo completo.

Ejecutar una sola vez como usuario normal en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --dhcp-supplicant artifacts/unicast-stage179
```

Contraseña de Especial solo en los dos prompts locales ocultos. Compartir salida completa sin contraseña. Captura simultánea; puede desconectar en0. No repetir174 ni179 si se detiene. Aún no se comprobó este ARP por radio; no declarar unicast ni avance adicional hasta analizar resultados.

[Preparación y hashes](supplicant-stage179-observation.json).

Resultado posterior: completado y verificado en180. ARP pairwise y replay local aprobados; contexto CLEAN con206 secuencias libres. Marcador179 consumido: no repetir el comando.
