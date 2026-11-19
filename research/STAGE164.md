# Etapa164 — recuperación fría tras el lease de162

2026-11-24. Se observó la ausencia USB del Archer y después una sesión nueva como instalador. Tras la expulsión local, aún seguía como instalador; una expulsión USB única no recibió CSW válido, pero la enumeración Wi-Fi y la inspección posterior confirmaron perfil frío. La recarga verificó firmware y archivó el contexto DIRTY de162 antes de publicar época12fc87ad-9c12-428d-9ca3-a060ded67535.

Bootstrap completo con13 redes, ocho comandos RFK de canal9 y contexto pasivo creado/retirado correctamente. Especial observado en canal9 con70 beacons y ocho probe responses de tráfico recibido, sin probes propios. Postflight210 registros correctos, TX pausado, contexto/fence CLEAN y215 secuencias libres. No se ejecutó otro ensayo DHCP ni se reutilizaron reservas.

El ACK de lease de162 y su auditoría163 siguen válidos como evidencia histórica. La recarga no demuestra que el servidor procesara RELEASE. El reporte TX de datos ausente sigue pendiente de diagnóstico; no se declara conexión utilizable ni interfaz de macOS.

[Observación y hashes](recovery-stage164-observation.json).
