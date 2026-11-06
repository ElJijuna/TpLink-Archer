# Etapa116 — contraste del descriptor de datos preparado

2026-11-06. Preparado, no ejecutado. El binario ARM/Intel incorpora la corrección de SW_MLD para datos identificada en113; gestión conserva su valor original. La regresión comprueba el DWORD3 del constructor real en ambos modos. Suites C++ y120 pruebas Python aprobadas.

La nueva instrumentación lee contadores PHY antes/después de cada fase, incluida la salida por ausencia de respuesta, antes de la retirada del contexto. No atribuye por sí sola emisión RF ni equivale a TX_DONE. Es importante conservar estas mediciones antes de que restauraciones posteriores impidan reconstruir el delta.

La admisión real sin TX comprobó la época115 activa, identidad/ledger intactos, contexto CLEAN,215 secuencias libres, evidencia física y hashes de fuentes/binario. El marcador nuevo116 aún no existe. Secuencias802.11 nuevas28–33; reporte de limpieza2 dentro de la época nueva. H2C41–49 solo se reservarán al ejecutar. Los marcadores e identidades anteriores no se reutilizan ni eliminan.

El contraste mantiene autenticación/asociación→M1→M2→M3 verificado→M4→un DHCP DISCOVER cifrado→OFFER CCMP coincidente. No se declara seguridad solo por USB/M3/M4; exige OFFER autenticado y correlacionado. Sin DHCP REQUEST, lease ni interfaz del sistema. Una fase incierta conserva DIRTY y no habilita reenvíos automáticos. CAM NO_LINK y el resto de la ruta BE aún pueden requerir trabajo adicional: no se da por corregida físicamente la ausencia de M3.

Ejecutar una sola vez en Terminal normal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v
python3 tools/external_probe_capture.py --wpa2-recovered artifacts/wpa2-stage116
```

Introducir la contraseña de Especial únicamente en el prompt local oculto. No se conservó la anterior. El coordinador usa un pipe anónimo, sin contraseña en archivos/argumentos/entorno; solo el helper de captura usa sudo. en0 puede desconectarse. Compartir la salida completa sin la contraseña. No repetir comandos de etapas anteriores.

Evidencia: [observación116 y hashes](wpa2-stage116-observation.json).
