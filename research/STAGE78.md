# Etapa78 — arranque eléctrico recupera RFK y barrido

2026-10-30. Mac ARM. Avance50 %. El flujo pasivo volvió a completar físicamente.

## Instalador y enumeración

El usuario confirmó desconexión/reconexión y después DKIOCEJECT=0 para el
CD virtual verificado como disk16. El modo seguía en0bda:1a2b. El envío normal
allow-removal y una recuperación BOT recibieron CSW con etiquetas distintas
de la enviada; ambos se rechazaron antes de START STOP UNIT.

Se añadió `--eject-only-installer` al helper mode-switch. Selecciona el mismo
dispositivo/interfaz verificados y envía una sola orden START STOP UNIT con
LOEJ, sin exigir antes allow-removal. Mantiene íntegra la validación de CSW.
También recibió etiqueta incorrecta y salió2; **no se considera éxito SCSI**.
Posteriormente la enumeración verificó37ad:0104 y el acceso de registros
funcionó. No se atribuye la transición exclusivamente a una de esas órdenes.

## Contraste desde frío

Los controles iniciales tenían BBMCU boot-ready0 y resets sin liberar.
El coordinador cold-start completó0 en `artifacts/cold-rfk-workflow-stage78`:
inspect → boot → radio → initial_rfk → tx_rfk → scan → sweep.
El RFK inicial utilizó recepción asíncrona y el multiplicador USB4x de etapa77.
PRE_NOTIFY/DACK/RXDCK devolvieron las confirmaciones exigidas. La secuencia
de calibración TX completó sus comandos con los límites de calidad ya
documentados para DPK/TSSI; no se afirma calidad RF completa.

| Canal de recepción | Entradas | Gestión aceptada | FCS erróneos |
|---|---:|---:|---:|
|1|19|110|0|
|6|7|96|0|
|11|8|94|0|

Informe conjunto: **34 BSSID distintos /34 pares BSSID-SSID**, con igualdad
de los tres replays offline. El escaneo inicial separado tuvo21 entradas,
que no se suman a las anteriores. La variación frente a otras capturas no
demuestra cambios del inventario de redes; son ventanas cortas de observación.

Retorno a canal1 verificado, scheduler TX pausado y contexto CLEAN. Consumo
de28 secuencias:251→223. El marcador de la prueba de trama TX sigue presente:
**no hubo nueva solicitud de sondeo ni asociación**. Las calibraciones pueden
incluir actividad RF interna; no se presenta el flujo como ausencia total de RF.

## Conclusión y límites

El reinicio eléctrico más carga fría recuperó el funcionamiento que no se
obtuvo mediante los resets en caliente de etapas76/77. Eso no identifica
el registro o estado retenido responsable. Tampoco aísla el efecto individual
del receptor asíncrono o del multiplicador de plazo, ya presentes en el último
contraste caliente que falló. No se declara reparado el reset en caliente.

La siguiente prueba de TX sigue requiriendo resolver el límite de reintentos
y gestionar de forma explícita el marcador del ensayo anterior; no se elimina
para repetir automáticamente. Intel/Linux y conexión Wi-Fi siguen pendientes.

## Evidencia

[cold-recovery-stage78-observation.json](cold-recovery-stage78-observation.json)
contiene hashes, conteos y estados finales. Logs locales:
`artifacts/cold-mode-switch-stage78{,b,c}.*`, `cold-preflight-stage78.*` y
`artifacts/cold-rfk-workflow-stage78/` con binarios, manifiesto y capturas.
Build universal correcto; `make test` pasa (78 tests Python y suites C++).
Log `artifacts/cold-rfk-tests-stage78.log`.
