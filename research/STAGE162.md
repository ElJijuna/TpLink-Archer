# Etapa162 — ensayo supplicant ligado a la recuperación actual

2026-11-24. La preparación159 nunca se ejecutó y el Archer reapareció como instalador. Este intento la sustituye, ligado a recuperación161, UUID2f5c0f06-7948-4c76-a387-782c48a6f425, contexto/fence CLEAN y215 secuencias libres.

Mantiene las respuestas M2 acotadas a tres M1 nuevos con el mismo ANonce y replay creciente, antes de M3. Usa slots y secuencias Wi-Fi propios, sin reinstalar claves. Después permite una retransmisión DISCOVER con mismo XID y PN/secuencia nuevos; solo continúa REQUEST/ACK si obtiene OFFER. Un ACK debe coincidir con transacción, MAC, servidor y dirección y contener lease no nulo. Intenta RELEASE y desautenticación con reportes distintos; TX_DONE no confirma procesamiento de RELEASE por el servidor.

Reserva secuencias Wi-Fi100–111, H2C41–49 y reportes13/14 dentro de la época nueva. Marcador propio `37ad-0104-special-channel9-eapol-retry-stage162-attempted`, todavía ausente. El coordinador no relanza automáticamente un ensayo; los límites de respuestas M1 y retransmisión DHCP se registran aparte. No configurar IP/rutas ni crear interfaz de red forma parte de este ensayo. Plazo30s, gracia3s de desautenticación, entrega a nativo antes de8s de captura y captura máxima45s; los presupuestos individuales se recortan al plazo global.

Binario supplicant, calibrador y auditor compilados ARM/Intel. Suites C++ y130 pruebas Python aprobadas. Admisión real comprobada sin prompt, USB, consumo de marcador o TX nuevos. Todavía falta comprobar el efecto por radio y completar la tarea de conexión utilizable. Avance físico conservado en64 %.

Ejecutar una sola vez como usuario normal en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --dhcp-supplicant artifacts/supplicant-stage162
```

Contraseña de Especial solo en los dos prompts ocultos locales. Compartir salida completa sin contraseña. Puede desconectar en0 y concederse un lease temporal que se intentará liberar. No repetir159/148 ni el nuevo comando si falla; conservar reservas y resultados para análisis.

[Preparación y hashes](supplicant-stage162-observation.json).
