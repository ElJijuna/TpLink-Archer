# Etapa105 — intento WPA2/CCMP preparado, pendiente por radio

2026-11-04. El estado físico sigue pausado en canal9:210 registros coinciden con el plan, concesión original0x01775774, sin nuevos H2C ni tramas. Contexto y transición de época CLEAN; ledger104 consumidas/152 libres. La admisión contrasta los hashes de la evidencia104 y conserva los marcadores anteriores.

Se implementaron derivación PMK/PTK, M2, verificación MIC/replay/ANonce de M3, desempaquetado AES y M4 para el perfil acotado WPA2-PSK/CCMP de Especial. CCMP software valida MIC antes de entregar plaintext o avanzar replay; contadores separados por clave/grupo/TID y PN TX sin reutilización. Pruebas con vectores PBKDF2, RFC3394, RFC3610 y fixtures independientes de Python/OpenSSL usando solo la contraseña pública ficticia `password`.

El intento nuevo reserva secuencias802.11 16–21 y usa el tag de reporte0, antes no utilizado, exclusivamente para desautenticación. El campo de reporte es de4 bits; la fuente rtw89 indexa `tx_rpt->skbs[sw_define]` sin excluir0 (`artifacts/rtw89-mac.c:5838`). Los envíos anteriores no solicitan reporte: requieren la respuesta de la fase siguiente para confirmar avance. Una respuesta ausente o transporte incierto conserva la reserva y no reenvía automáticamente. Los envíos se limitan además a20 segundos desde el inicio nativo, con hasta3 segundos adicionales para desautenticación; el coordinador solo entrega dentro de los primeros20 segundos de la captura de45 segundos. Los nueve H2C de contexto se reservan al ejecutar; todavía no se han consumido.

Secuencia: autenticación, asociación, M1→M2→M3 verificado→M4, un DHCP DISCOVER cifrado, espera acotada de OFFER coincidente y desautenticación cuando el estado previo esté confirmado. Gestión por USB05/MGQ, EAPOL/datos por USB09/BE. M4 y DISCOVER usan la misma cola con una ventana de100ms entre ellos. La ruta BE y este intercambio completo **aún no están verificados físicamente**. Si una fase queda incierta, el contexto permanece DIRTY para análisis; no se promete limpieza automática de un envío sin confirmación.

Solo una oferta DHCP autenticada por CCMP, con XID y MAC coincidentes, permite declarar el camino de datos. No se envía DHCP REQUEST, no se adquiere lease, no se crea interfaz del sistema ni se afirma conexión utilizable. Un M3 válido o un M4 entregado por USB no bastan.

La contraseña se solicita oculta en Terminal, pasa por un pipe anónimo heredado y no se escribe en argumentos, variables de entorno, capturas ni logs. El entorno contiene únicamente el número de descriptor. El código nativo borra buffers de claves; las cadenas temporales de Python no tienen garantía de borrado de memoria. Las capturas preservan tráfico EAPOL y CCMP tal como viaja por radio, sin serializar claves derivadas ni plaintext recibido.

Ejecutar una sola vez, como usuario normal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v
python3 tools/external_probe_capture.py --wpa2-once artifacts/wpa2-stage105
```

La contraseña Wi-Fi se introduce solo en ese prompt local. Únicamente el helper de captura usa sudo. en0 puede desconectarse; la captura registra el fallo de restauración si ocurre. Compartir la salida sin la contraseña.

Validación y hashes: [observación105](wpa2-stage105-observation.json). Estado al preparar: compilación universal ARM/Intel, suites C++ y116 pruebas Python aprobadas; prueba física de WPA2/datos pendiente. DPK/TSSI, calidad RF completa, Intel físico, DriverKit e implementación Linux siguen abiertos.
