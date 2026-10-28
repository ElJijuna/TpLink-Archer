# Etapa64 — escaneo pasivo agregado en canal1

Fecha: 2026-10-27. Se implementó un agregador portable de observaciones
beacon/probe response, integrado en el ejecutor RX y disponible offline.
No retunea, no envía probe requests y no implementa asociación.

## Resultado sobre Mac ARM

El perfil asíncrono de etapa63 volvió a ejecutarse sin recargar firmware.
Capturó313 paquetes normales y produjo293 observaciones de gestión aceptadas:
238 beacons y55 probe responses. Se agruparon en17 entradas, con0 errores
FCS y0 desbordamientos. Los20 normales restantes no son aceptados por el
parser acotado; no se declaran corruptos a partir de ese rechazo.

`artifacts/context-rx-stage64/networks.json` contiene el resultado local.
El contexto se retiró, TX de red permaneció pausado, reserva final CLEAN,
salida0, stderr vacío y nueve peticiones USB canceladas/drenadas.
La salida del agregador en vivo es **idéntica** a su reproducción offline
sobre los369 archivos de transferencias recibidas.

Las capturas anteriores también se reproducen: etapa63 genera17 entradas y
257 observaciones; etapa63b genera15 entradas y268 observaciones, sin errores
FCS. Distintas duraciones/tráfico observado pueden cambiar qué entradas se
ven; no se afirma que estos conteos sean un inventario exhaustivo de redes.

## Modelo y límites

`include/archer/passive_scan.hpp` comparte el parser de gestión existente.
Primero valida límites de payload, descriptor sin errores, cabecera sin convertir
y política FCS appended. Calcula IEEE CRC32 antes de admitir una observación.
Los BSSID deben ser individuales y no nulos. Clave de agrupación:
BSSID + bytes SSID + canal DS anunciado opcional.

Cada entrada conserva conteos beacon/probe response, primera/última observación
por ordinal y último intervalo/capability observado. Los SSID se serializan en
hexadecimal para conservar bytes arbitrarios sin asumir UTF-8. Una variante
oculta y una anunciada permanecen separadas; también anuncios contradictorios
de SSID o canal. Por eso17 entradas no garantiza17 puntos de acceso distintos.
No se deduce seguridad WPA de capability ni se inventa RSSI.

El canal de recepción es el1 configurado por el perfil; el canal DS procede del
anuncio, no del descriptor RX. Puede diferir por recepción adyacente. No se
confunden estos dos campos. La ausencia de canal DS permanece null.
Las observaciones abarcan todas las fases de recepción, no solo los cinco
segundos de la ventana principal; los ordinales no son timestamps.

La tabla admite128 entradas. Una nueva clave al llenarse incrementa overflow
sin abortar la retirada del contexto; las claves existentes siguen acumulando.
El campo bad_fcs cuenta fallos de CRC32 de candidatos normales, mientras que
unsupported incluye rechazos de descriptor/política/framing o parser acotado.

## Uso y validación

```
make test context-rx-universal passive-scan-decode-universal
./build/archer-passive-scan-decode-universal --channel1-fcs-appended RX_DIRECTORY
```

La opción declara un perfil conocido, no descubre el canal ni la presencia de
FCS de una captura arbitraria. La herramienta offline exige archivos contiguos
rx-0.bin..rx-N.bin, entre1 y1024 transferencias de hasta20480 bytes. Rechaza
huecos en los nombres; los procesa por ordinal numérico.

El ejecutor en vivo escribe networks.json después del retiro reconocido del
contexto. Si falla esa escritura, informa del fallo sin volver a enviar H2C.
Los mensajes de progreso solo emiten conteos; SSID/BSSID quedan en el archivo
local. La captura cruda y los resultados completos permanecen en artifacts.

51 suites C++ y55 tests Python pasan; ambos ejecutores compilan arm64/x86_64.
El nuevo test usa el vector conocido CRC32 123456789=cbf43926, agrupación de
beacon/probe response, SSID binario, anuncios contradictorios, SSID vacío,
rechazo de FCS, política desconocida, límites y capacidad de tabla.
Solo se probó físicamente ARM; no se afirma una integración con macOS Wi-Fi.

Avance ponderado **48 %**:3/15 en escaneo/asociación por este escaneo pasivo
fijo; falta barrido coordinado de canales, asociación y seguridad. El siguiente
paso debe integrar el arranque y ciclo de vida reproducibles antes de ampliar
el barrido, conservando la evidencia de firmware y las reservas de secuencias.

Evidencia y hashes: [passive-scan-stage64-observation.json](passive-scan-stage64-observation.json).
