# Etapa163 — ACK DHCP observado; cierre pendiente

2026-11-24. El intento162 autenticó M3, envió M4 y recibió un OFFER coincidente a los3378ms del DISCOVER. Tras REQUEST, recibió un ACK DHCP autenticado y coincidente en12ms: el servidor reconoció un lease temporal. No se creó interfaz ni se configuró IP en macOS. El resumen nativo registra nueve tramas autenticadas, todas de grupo; la recepción física unicast sigue pendiente.

RELEASE completó su escritura USB, pero no apareció el reporte TX_DONE13 durante tres segundos. No se envió desautenticación. El ensayo terminó detenido y el contexto quedó DIRTY, con206 secuencias libres. No se confirma recepción ni procesamiento de RELEASE por el servidor; el lease podría persistir. No repetir162 ni RELEASE ni borrar reservas.

El análisis de las transferencias guardadas no encontró reportes TX, incluidos tags alternativos o tardíos. La captura independiente se guardó, pero la restauración del canal de en0 falló; reconectar el Wi-Fi del Mac si hace falta. La comprobación posterior validó210 registros con TX pausado, sin nuevos H2C ni transmisiones Wi-Fi.

Se amplió el auditor offline para autenticar el ACK y reproducir REQUEST/RELEASE desde OFFER/ACK, verificar su cifrado y el avance de PN. Esta comprobación valida los bytes guardados, no la entrega por radio ni la liberación del servidor. Pasaron132 pruebas Python, las suites C++ y compilación del auditor ARM/Intel; la regresión incluye corrupción de REQUEST y RELEASE. La auditoría de la captura real aún necesita la credencial local, que no se guardó.

Ejecutar como usuario normal, sin sudo:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
python3 tools/audit_wpa2_capture.py artifacts/supplicant-stage162/probe/capture artifacts/lease-offline-stage163
```

Contraseña solo en el prompt oculto local. No utiliza USB ni transmite; escribe metadatos sin claves ni plaintext. No hace falta desconectar el adaptador para esta auditoría. La conexión utilizable en macOS sigue incompleta.

[Evidencia y hashes](supplicant-stage163-observation.json).
