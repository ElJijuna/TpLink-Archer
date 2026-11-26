# Etapa194 — controles de arranque a través del nuevo transporte

Se añadió ReadBootControls a la clase DriverKit. Lee seis controles mediante ReadRegister, con guardia de llamada que impide finalizar Stop durante la observación. Ninguna salida se publica ante fallo parcial, sentinela o interrupción del transporte. Los resultados son clasificación de controles, no prueba de identidad de firmware ni permiso de reset/descarga.

La prueba reproduce valores de los primeros reads de FC/F0/1E0/84/D0/842C de recovery-stage188-boundary/reload.jsonl. Un caso running sintético está identificado por separado; no se inventa una lectura física D0 ausente del postflight. Se rechazaron sentinelas en cada campo, chip/cut incorrecto y estados ROM4/5/6/7/15. ASan/UBSan y el build/enlace universal DriverKit27.2 pasan; el verificador comprueba que la nueva API también es LOCALONLY.

No se invoca automáticamente en Start. No hubo lecturas USB reales ni escrituras, reset, firmware o Wi-Fi en esta etapa. La DEXT sigue sin instalarse. El motor completo requiere portar ventanas WLAN/RF y precondiciones, validación/propiedad de imágenes y reservas, descarga/confirmaciones y drenaje; no se sustituye por esta observación parcial.
