# Etapa49 — informe PHY hacia RMAC

Fecha: 2026-10-21. Se identificó una configuración del arranque Linux
posterior al CMAC que todavía no se aplicaba en el perfil actual:
`core.c:6700-6701` habilita PPDU y PHY report. La segunda llamada usa
`rtw89_mac_cfg_phy_rpt_be` (`mac_be.c:2595-2606`) para configurar RCR
PHY_RPT_SZ=MAC_AX_PHY_RPT_SIZE_8(1), HDR_CNV_SZ=0 y DRV_INFO_PHYRPT_EN=1.
Fuente fijada a `c03c1a14507dfaf8b856c2608f9ac216021fc222`.

`--diagnostic-phy-report` aplica esos tres campos en una ventana reversible,
sin alterar los demás bits. Usa el perfil base de recepción, con motor de
respuestas desactivado y scheduler TX pausado. Reinicia contadores PMAC para
la medición, como los otros perfiles diagnósticos. No cambia la configuración
persistente del inicializador ni envía tramas o comandos firmware.

## Observación física

Ventana ARM de cinco segundos:0 transferencias USB. Se verificó por readback
RCR20f3e000→20f3e010→20f3e000 y bit0 de DRV_INFO_OPTION0→1→0.
El PPDU_STAT existente se incluye en los snapshots sin modificarlo.
El ejecutor terminó restaurado, stderr vacío, scheduler TX en0.
Habilitar PHY report por sí solo no resolvió la entrega RX.

## Filtros UID y límites de la comparación

Se obtuvo la fuente primaria `mac80211.c` de la misma revisión, conservada en
`artifacts/rtw89-mac80211-stage49.c`. `rtw89_ops_configure_filter:308-378`
confirma el uso de A_BC/A1/BCN y SNIFFER_MODE en los caminos de escaneo y monitor.
No aporta una modificación de UID_FILTER que justifique variar ese campo.
El perfil conserva UID_FILTER=15, el valor predeterminado BE declarado en
`reg.h`. La ausencia de evidencia para cambiarlo no demuestra que el filtro
UID no intervenga ni que el contexto firmware/CAM sea suficiente.

También se identificó un paso aún ausente de la reconstrucción:
`rtw89_phy_dm_init` llama `rtw89_physts_parsing_init`; ajusta bitmaps de
información PHY y condiciones de reporte (`phy.c:7157-7213,8244`).
Esta etapa no ejecutó ese paso ni el resto del ciclo DM. Es el siguiente
contraste con respaldo de fuente, antes de atribuir el bloqueo a hardware.

Validación:40 suites C++ y46 tests Python; compilación ARM/Intel. Las pruebas
nuevas verifican campos efectivos, preservación de bits, restauración tras
fallos de cada escritura/callback y rechazo sin escrituras cuando TX está activo.
Hashes del binario ejecutado, fuentes adicionales y trazas en
[phy-report-stage49-observation.json](phy-report-stage49-observation.json).
Progreso ponderado40%: todavía no hay tramas entregadas por USB.
