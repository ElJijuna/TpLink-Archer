# Etapa166 — reportes ACQ desactivados en el contexto anterior

2026-11-24. El intento165 autenticó WPA2 y recibió un DISCOVER reflejado y un OFFER coincidente, pero no recibió el reporte TX12. Ocho tramas de grupo autenticadas, cero rechazos. La barrera se detuvo antes de retransmitir o enviar REQUEST: este ensayo no adquirió lease. Contexto DIRTY; conservar reservas. El replay USB confirmó cero reportes TX entre269 paquetes normales,570 metadatos PPDU y ocho C2H de control.

La tabla CMAC capturada en tx-4.bin contiene W0=0x00200004. En la fuente fijada fw.h, CCTLINFO_G7_W0_ACQ_RPT_EN es bit20 y MGQ_RPT_EN bit21. El valor activa gestión y desactiva datos; fw.c default_cmac_tbl_g7 también habilita solo gestión por defecto. La ausencia de ACQ es una diferencia concreta que puede explicar los reportes ausentes de datos, no una prueba todavía de la causa RF.

Se añadió opt-in ACQ al generador y ciclo real de contexto; el contraste del supplicant cambia únicamente bit20 a W0=0x00300004 y mantiene la máscara que incluye ese bit. Los demás contextos conservan el valor anterior. No cambia tasa, cola, claves, paquete Wi-Fi ni límite de reintentos. Conserva la barrera DISCOVER TX_DONE previa a REQUEST. La regresión recorre el constructor real y sus ACK hasta la tabla, compara todos los bytes y verifica la máscara; no permite ACQ sin reportes de gestión.

Pasaron133 pruebas Python, suites C++ y compilación ARM/Intel. No se declara corrección física. La admisión futura apunta a recuperación167 y marcador168, aún no preparados; no ejecutar ensayos anteriores. Ausencia USB observada y nueva sesión instalador verificada. La expulsión está pendiente de autenticación sudo local.

[Evidencia y hashes](acq-report-stage166-observation.json).
