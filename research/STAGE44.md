# Etapa44 — parser de gestión 802.11

Se añadió `include/archer/beacon.hpp` y su integración offline en `archer-decode`.
El subconjunto admitido es beacon y probe-response normales, versión0,
cabecera24 y campos fijos12. Rechaza fragmentación, protección, bits DS,
cabecera de control adicional, elementos truncados, SSID ausente/duplicado
o mayor32 y DS Params duplicado/incorrecto. SSID conserva bytes arbitrarios;
un SSID vacío se admite. Otros elementos se omiten respetando sus límites.
El canal DS es información anunciada por la trama, no una medida del receptor.

Fuente primaria: Linux commit `c03c1a14507dfaf8b856c2608f9ac216021fc222`,
[`include/linux/ieee80211.h`](https://github.com/torvalds/linux/blob/c03c1a14507dfaf8b856c2608f9ac216021fc222/include/linux/ieee80211.h),
constantes FC/subtipos, `struct ieee80211_mgmt` y elementos SSID/DS.
Copia local `artifacts/linux-ieee80211-stage44.h`, SHA256
`a65c9f90c87f66509a8fb31c0a77bdbbfae019cd0199edf131cbd798f140bdaa`.
Las restricciones adicionales son decisiones conservadoras del parser,
no una afirmación de que todos esos formatos estén prohibidos por 802.11.

Uso: `archer-decode RX_TRANSFER_FILE --fcs-appended` o `--fcs-stripped`.
La opción es una declaración del operador que debe apoyarse en la configuración
de adquisición. Sin opción, FCS permanece desconocido y no se interpreta gestión.
Solo se admiten descriptores largos sin conversión ni errores CRC/ICV.
La salida expone clase de gestión, longitud SSID, intervalo y canal anunciado;
omite SSID/BSSID. No calcula el checksum FCS; quitarlo no verifica su integridad.
La ausencia de resultado de gestión puede significar otro subtipo o una trama
rechazada, y no demuestra ausencia de redes.

Validación: 36 suites C++ y46 tests Python; compilación del CLI ARM/Intel;
parser bajo AddressSanitizer/UndefinedBehaviorSanitizer; integración CLI con
beacon sintético y FCS explícito. Registros: `artifacts/tests-stage44-beacon-final.txt`.
Se corrigió además la inicialización por defecto de `BeRxMetadata::packet`.
No se accedió al USB. No hay aún ventana RX pasiva validada con CMAC actual,
escaneo multicanal, asociación o interfaz de red. Progreso ponderado:40%.
