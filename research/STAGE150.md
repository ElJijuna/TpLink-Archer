# Etapa150 — intento148 detenido tras M2; comparación de credencial preparada

2026-11-17. Se verificaron ejecutable, stdout/stderr y PCAP del intento148 contra sus manifiestos. Autenticación/asociación aceptadas, M1 recibido y M2 enviado; no llegó M3 en3015 ms. No hubo M4, DISCOVER, retransmisión DHCP, REQUEST/ACK o RELEASE. El contraste de retransmisión todavía no se ha ejercitado por radio.

Este resultado no demuestra por sí solo contraseña incorrecta. El usuario confirma que la clave de Especial no cambió desde el intercambio exitoso138. Eso permite usar su M3 autenticado como referencia histórica para comprobar una credencial introducida localmente, sin transmitir ni reutilizar la clave escrita antes en el chat.

El auditor offline admite ahora `--compare-attempt`: primero reproduce M2 y autentica M3/M4 de la referencia con la clave local. Solo después reconstruye M2 del intento comparado con su M1 y SNonce registrados. Emite `reference_m3_authenticated` y `attempt_m2_reproduced`; no imprime claves o plaintext. Un M2 distinto señala que esa credencial no reproduce el intento, no un fallo de recepción M3. Si coincide, la falta de M3 permanece sin explicar. Una clave que no autentica la referencia detiene la auditoría antes de producir una conclusión sobre el intento.

Se guardan hashes de las entradas de referencia y comparación por separado. Las pruebas sintéticas cubren coincidencia, MIC de M2 diferente y clave de referencia incorrecta sin salida de datos. Suites C++ y126 pruebas Python pasan; auditor ARM/Intel compilado. La comparación real sigue pendiente de la entrada local de contraseña.

Ejecutar como usuario normal, **sin sudo**:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
python3 tools/audit_wpa2_capture.py artifacts/wpa2-stage138/probe/capture artifacts/credential-check-stage150 --compare-attempt artifacts/dhcp-stage148/probe/capture
```

Introducir la contraseña correcta de Especial solo en el prompt local oculto. Compartir salida completa sin contraseña. Solo archivos, sin USB ni TX. No desconectar ni repetir148 para esta comprobación.

Marcador148 consumido, contexto DIRTY y206 secuencias libres. Postflight210 registros correcto y TX pausado. La captura terminó, pero falló restaurar el canal original de en0; reconectar Wi-Fi del Mac si hace falta. Ningún TX nuevo en150; avance físico64 %, tarea todavía incompleta.

[Observación y hashes](dhcp-stage150-observation.json).
