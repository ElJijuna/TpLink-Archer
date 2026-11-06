# Etapa130 — contraste de escucha DHCP ampliada admitido

2026-11-06. El intento nuevo mantiene DISCOVER broadcast de122 y la instrumentación de datos autenticados; amplía únicamente el presupuesto de escucha DHCP de tres a diez segundos. Conserva un solo envío, sin retransmisiones. El plazo total nativo de veinte segundos puede acortar esa ventana; solo limpieza dispone de tres segundos adicionales. Los registros de espera indican presupuesto solicitado, duración real y respuesta recibida.

La espera más larga contrasta temporización, no corrige una causa demostrada. El servidor sigue pudiendo aplicar políticas específicas y todavía no hay recepción pairwise física confirmada. OFFER autenticado y coincidente sigue siendo el criterio de finalización; DISCOVER reflejado no cuenta como oferta ni permite liberar reservas.

## Preparación verificada

Recuperación129, UUID51e59f80-528b-446c-b79f-3c9ac307bca2, contexto/fence CLEAN y ledger41/215. Secuencias Wi-Fi40–45, H2C41–49 y reporte de limpieza4. Marcador nuevo `37ad-0104-special-channel9-wpa2-recovered-stage130-attempted`, todavía ausente. Se exige conservar los marcadores históricos116 y122.

Compilación universal ARM/Intel correcta, suites C++ y123 pruebas Python aprobadas, incluyendo los plazos deterministas preparados125. La admisión real pasó sin USB, prompt, reservas nuevas o TX. No se ha ejecutado este contraste por radio. Los comandos antiguos122 no deben volver a ejecutarse: el coordinador vigente admite exclusivamente130.

## Ejecución local única

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --wpa2-recovered artifacts/wpa2-stage130
```

Introducir contraseña de Especial solo en el prompt local oculto. Captura simultánea, autenticación/asociación WPA2, M2/M4 y un DISCOVER cifrado; observa OFFER y trata de desautenticar si la entrega está confirmada. No adquiere lease ni crea interfaz de red. Puede desconectar en0; sin reintento automático. Compartir salida completa sin contraseña; un fallo necesita análisis, no repetición del comando.

[Preparación y hashes](wpa2-stage130-observation.json). Avance estimado conservado en60 % hasta obtener evidencia nueva.
