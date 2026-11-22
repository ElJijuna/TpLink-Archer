# Etapa174 — ensayo con una retransmisión REQUEST

2026-11-24. Preparación ligada a recuperación173, épocaf99dbe27-5d10-4876-bb30-aa835c07f33e, contexto/fence CLEAN y215 secuencias libres. Marcador174 nuevo;170 permanece consumido. Binario y auditor ARM/Intel; suites C++ y134 pruebas Python aprobadas. Hashes y admisión actual comprobados sin credenciales, USB ni TX adicionales.

Mantiene ACQ_RPT_EN, que en170 habilitó TX_DONE de DISCOVER. Exige ese reporte antes de retransmitir DISCOVER o enviar REQUEST. El primer REQUEST solicita tag11; solo si obtiene TX_DONE/count1 y sigue sin ACK/NAK se permite una retransmisión REQUEST con mismo XID, servidor y dirección, PN/secuencia nuevos y tag10. Espera inicial aleatoria3–5s; la segunda usa el resto del presupuesto REQUEST total10s, sujeto al plazo global30s. Drena RX otra vez antes de decidir retransmitir. NAK autenticado del servidor elegido detiene el ensayo; no reinicia a INIT ni relanza la captura.

Hasta tres respuestas M1 con replay creciente, una retransmisión DISCOVER y una REQUEST dentro del ensayo. Layoutv2 y secuencias Wi-Fi100–112; REQUEST9, retry10, RELEASE11, deauth12. Reportes DISCOVER12, REQUEST11, retry10, RELEASE13, deauth14. Los validadores exigen identidad, orden, reportes y metadatos frescos. RELEASE/TX_DONE no confirma liberación del servidor. No configura IP, rutas ni interfaz macOS. El efecto físico de la retransmisión REQUEST todavía no está comprobado.

Ejecutar una sola vez como usuario normal en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --dhcp-supplicant artifacts/supplicant-stage174-capture
```

Contraseña de Especial solo en los dos prompts locales ocultos. Compartir salida completa sin contraseña. Puede desconectar en0 y solicitar lease temporal que intentará liberar si recibe ACK. No repetir170 ni174 si se detiene; conservar reservas y capturas.

[Preparación y hashes](supplicant-stage174-observation.json).

Incidencia de directorio antes del ensayo: la ruta original artifacts/supplicant-stage174 existe vacía, sin paired.json/probe ni marcador174. Contexto CLEAN y ledger sin cambios (215 libres); admisión revalidada. No hubo intento Wi-Fi registrado. Se conserva esa carpeta y se usa la ruta nueva artifacts/supplicant-stage174-capture para el mismo marcador174, sin borrar ni reutilizar reservas. Evidencia: artifacts/supplicant-stage175-directory-audit.json.
