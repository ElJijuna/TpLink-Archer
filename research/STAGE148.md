# Etapa148 — ensayo DHCP con una retransmisión acotada admitido

2026-11-17. Ligado a recuperación147 y UUID8632af42-2107-421a-be29-06401f34d4c4. Contexto/fence CLEAN,215 secuencias libres. Marcador nuevo `37ad-0104-special-channel9-dhcp-lease-stage148-attempted`, todavía ausente; se comprueba conservar141 consumido y los marcadores WPA2 anteriores.

Reserva secuencias Wi-Fi67–75 y H2C41–49. DISCOVER inicial usa71; retransmisión opcional72, REQUEST73, RELEASE74 y desautenticación75. Si llega OFFER antes de reenviar, se omite72. Reportes distintos: RELEASE9 y desautenticación10.

La primera espera es aleatoria3–5s; una única retransmisión puede seguir con espera7–9s. Conserva XID, actualiza secs y usa PN CCMP/secuencia Wi-Fi nuevos. No relanza un experimento fallido. Los manifiestos distinguen `automatic_retry=false` del coordinador y `protocol_discover_retransmissions_max=1` del protocolo. No se ha probado todavía esta política por radio ni atribuido la falta de OFFER a una causa.

Si llega OFFER, continúa con REQUEST; solo acepta ACK autenticado que coincida con XID, chaddr, servidor y dirección y contenga lease no nulo. Si recibe ACK, intenta RELEASE unicast y exige su TX_DONE antes de desautenticar. RELEASE no tiene confirmación de servidor: puede quedar lease pendiente hasta expirar si falla la entrega o procesamiento. No configura IP/rutas ni crea interfaz del sistema.

Límite total nativo30s, gracia3s exclusiva de desautenticación, captura45s. Admisión real verificada sin consumir marcador ni TX. Compilación ARM/Intel, suites C++ y125 pruebas Python aprobadas. El avance físico sigue64 %; REQUEST/ACK aún no se ha ejercitado en radio.

Ejecutar una sola vez como usuario normal en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --dhcp-lease artifacts/dhcp-stage148
```

Contraseña de Especial únicamente en los dos prompts locales ocultos. Compartir salida completa sin contraseñas. Puede desconectar en0; no repetir141 ni el nuevo comando si falla, ni borrar reservas.

[Preparación y hashes](dhcp-stage148-observation.json). [Diseño y validación de la retransmisión](STAGE144.md).
