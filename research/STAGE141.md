# Etapa141 — REQUEST/ACK/RELEASE preparado para ensayo único

2026-11-11. Ligado a recuperación140 y UUIDa18950a3-9b75-4a74-b999-75be5760d9e3. Contexto/fence CLEAN,215 secuencias libres. Marcador nuevo `37ad-0104-special-channel9-dhcp-lease-stage141-attempted`, todavía ausente; se conservan los marcadores anteriores116/122/133/138.

Secuencias Wi-Fi58–65; H2C41–49; reporte RELEASE7 y desautenticación8. Ocho fases: autenticación, asociación, M2, M4, DISCOVER, REQUEST, RELEASE y desautenticación. Solo acepta ACK autenticado coincidente del servidor/dirección seleccionados con lease no nulo. REQUEST y RELEASE se emiten una vez; si queda entrega incierta no se permite repetir ni liberar manualmente el contexto.

La captura tiene límite45s; el ensayo nativo conserva límite20s y gracia3s exclusiva de desautenticación. Esperas OFFER/ACK de hasta10s pueden recortarse por el límite global. No hay retransmisiones ni instalación de dirección o ruta en el sistema. Si llega ACK, el servidor habrá concedido un lease temporal; se intentará RELEASE unicast a la MAC de origen autenticada del ACK y se exigirá TX_DONE. DHCP no confirma RELEASE: no se declara liberación procesada por servidor. Si se interrumpe, el lease podría persistir hasta expirar. No se utiliza esa dirección para tráfico IP adicional.

Admisión real correcta, sin consumir marcador ni acceder al USB. Suites C++ y125 pruebas Python pasan; binario ARM/Intel compilado. El ensayo físico sigue pendiente y no se ha completado la tarea de conexión utilizable.

Ejecutar una sola vez, como usuario normal en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --dhcp-lease artifacts/dhcp-stage141
```

Introducir la contraseña de Especial solo en los dos prompts ocultos locales. Compartir salida completa sin contraseñas. Puede desconectar en0. Conservar todos los resultados y reservas; no repetir comandos138 anteriores ni el nuevo comando si falla.

[Preparación y hashes](dhcp-stage141-observation.json).
