# Etapa159 — ensayo de respuestas M1 y DHCP admitido

2026-11-17. Ligado a recuperación158 y UUID018e8cb1-df19-4467-a428-b824e63711f3. Contexto/fence CLEAN,215 secuencias libres. Marcador nuevo `37ad-0104-special-channel9-eapol-retry-stage159-attempted`, todavía ausente. No reutiliza148 ni la preparación155 bloqueada.

Reserva secuencias Wi-Fi88–99 y H2C41–49. Slots3–5 opcionales responden a hasta tres M1 posteriores al inicial, con ANonce constante y replay creciente. Cada M2 lleva replay/MIC propios y secuencia Wi-Fi nueva; no usa CCMP ni reinstala claves. Si M3 llega directamente, omite esos slots. El hallazgo151 identifica esta ruta faltante, pero su efecto físico todavía no está demostrado.

Tras M3/M4, DISCOVER ocupa slot7; slot8 es una retransmisión DHCP opcional con mismo XID y PN/secuencia nuevos. REQUEST9, RELEASE10 y desautenticación11; reportes RELEASE13 y desautenticación14. Un ACK debe coincidir con transacción, MAC, servidor y dirección y contener lease no nulo. Si llega, intenta liberar el lease; TX_DONE no confirma que el servidor haya procesado RELEASE. No configura IP, rutas ni interfaz del sistema.

Plazo total30s y gracia3s exclusiva de desautenticación; entrega al nativo antes de8s de captura, cuya duración máxima es45s. Las esperas individuales pueden recortarse por el plazo global. El coordinador no relanza automáticamente un intento fallido; los presupuestos de respuestas M1 y retransmisión DISCOVER se registran por separado.

Binario supplicant, calibrador y auditor compilados ARM/Intel; suites C++ y130 pruebas Python aprobadas. Admisión real correcta sin prompt, USB, consumo de marcador o TX nuevos. La recepción de ACK y los nuevos caminos de retransmisión siguen pendientes de prueba por radio.

Ejecutar una sola vez como usuario normal en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --dhcp-supplicant artifacts/supplicant-stage159
```

Contraseña de Especial únicamente en los dos prompts locales ocultos. Compartir salida completa sin contraseñas. Puede desconectar en0, solicitar lease temporal e intentar RELEASE; si falla, conservar reservas y resultados, no repetir comandos159/148 ni borrar archivos.

[Preparación y hashes](supplicant-stage159-observation.json). Avance físico64 %, tarea todavía abierta.
