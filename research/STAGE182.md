# Etapa182 — ensayo ICMP sin reconexión innecesaria

2026-11-25. Ligado al resultado180: contexto/fence CLEAN,206 secuencias libres y misma épocaabec701b-e225-4e2c-a716-8dc1ee4dc5c6. Se verificaron210 registros actuales sin TX nuevos. Admisión exige ledger exacto, ARP pairwise/replay/cierre anteriores, hashes y marcadores conservados; no basta una marca CLEAN aislada. No se recargó firmware ni se reutilizan secuencias de179.

Marcador182 nuevo, Wi-Fi200–214 y nueve H2C nuevos50–58. Obtiene otro lease temporal mediante el flujo validado, resuelve el gateway con una sola solicitud ARP y envía un único Echo IP a esa MAC/IP. Exige reply pairwise con checksums e identificador/cookie correctos y rechazo local de replay. Espera Echo hasta3s dentro del plazo global30s. Si los envíos son terminales, intenta RELEASE/desautenticación y retirada incluso ante ausencia de reply. RELEASE no confirma liberación del servidor. No crea interfaz ni configura IP/rutas en macOS.

142 pruebas Python, suites C++ y ARM/Intel aprobados; falta contraste físico IP. No se relanza automáticamente el ensayo completo ni se retransmite ARP/Echo. Las retransmisiones DHCP previas siguen limitadas a una por fase, condicionadas a sus reportes TX, y NAK detiene REQUEST.

Ejecutar una sola vez como usuario normal en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --dhcp-supplicant artifacts/icmp-stage182
```

Contraseña de Especial solo en los dos prompts locales ocultos. Compartir salida completa sin contraseña. Captura simultánea; puede desconectar en0. No repetir179 ni182 si se detiene y conservar reservas/capturas.

[Preparación y hashes](supplicant-stage182-observation.json).
