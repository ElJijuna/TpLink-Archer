# Etapa171 — reporte TX de datos verificado; ACK DHCP pendiente

2026-11-24. El intento170 activó ACQ_RPT_EN en la tabla CMAC; el paquete capturado contiene W0=0x00300004. Comparado con165, solo cambia bit20 dentro del payload de tabla, conservando máscaras y restantes campos. El replay encontró C2H1/9/1 en rx-142.bin, tag12, estado TX_DONE0, count1. En165 no había reporte; este contraste confirma que activar ACQ habilita el reporte de datos para DISCOVER en el perfil físico actual. No implica validación de todos los tipos de paquete ni del reporte RELEASE, que no se ejecutó aquí.

WPA2 autenticado, DISCOVER reflejado y OFFER coincidente recibidos; la espera inicial terminó a3379ms con reporte y OFFER. Se envió REQUEST fase9 por USB, pero no se aceptó ACK durante10003ms. No se envió RELEASE ni desautenticación. Resumen:20 tramas autenticadas, cero rechazos CCMP, todas de grupo, un OFFER. Contexto DIRTY y206 secuencias libres. No se observó lease reconocido, pero el estado de lease del servidor es desconocido tras REQUEST: no afirmar que no existe.

Hashes de helper, stdout, stderr y PCAP verificados contra sus manifiestos. Replay USB:820 paquetes normales,824 metadatos PPDU y nueve C2H, incluido el reporte TX12. La captura independiente fue guardada; monitor_exit1 no invalida el PCAP por sí solo. No reutilizar170 ni solicitudes anteriores.

Antes de otro ensayo, auditar la captura real como usuario normal, sin sudo ni USB/TX:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
python3 tools/audit_wpa2_capture.py artifacts/supplicant-stage170/probe/capture artifacts/dhcp-offline-stage171
```

Contraseña de Especial únicamente en el prompt oculto local. El auditor autentica lo guardado, reproduce REQUEST y clasifica DHCP sin guardar claves ni plaintext. Esta auditoría puede distinguir respuestas presentes/rechazadas de ausencia en lo capturado, pero no pérdida previa al receptor ni estado interno del servidor. No hace falta desconectar el adaptador para auditar.

[Evidencia y hashes](acq-report-stage171-observation.json).

Auditoría local terminada:22 tramas autenticadas y cero rechazos, un OFFER coincidente, REQUEST reproducido y una trama REQUEST reflejada con XID/chaddr coincidentes. Ningún ACK ni NAK entre las tramas guardadas. Hashes de helper, entradas y salidas verificados. No prueba ausencia de respuesta en el servidor ni descarta pérdida antes de captura. Sin USB ni TX durante auditoría. No hace falta repetirla.
