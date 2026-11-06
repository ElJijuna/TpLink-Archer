# Etapa117 — M3 autenticado, M4/DISCOVER enviados; auditoría offline preparada

2026-11-06. El directorio116 ya contiene un intento registrado con marcador consumido y salida2. Por eso no debe volver a ejecutarse el comando. El fragmento de traceback en `mkdir` es compatible con intentar usar ese directorio existente; falta la última línea para atribuirlo con certeza. Se añadió un error claro que conserva resultados y se detiene antes de credenciales, admisión o USB. Una regresión comprueba esa frontera; también se verificó el mensaje sobre el directorio real sin alcanzar el backend.

## Resultado real del intento116

El contexto recibió todos los ACK exigidos. Especial aceptó autenticación y asociación, estado0/AID1. Se recibió M1, M2 se entregó por USB09 y **M3 pasó la verificación criptográfica nativa**, incluido MIC, replay, ANonce y desempaquetado de clave. Se emitieron M4 (195 bytes USB) y un DHCP DISCOVER cifrado (440 bytes USB). La espera falló después, al no confirmar OFFER; no se transmitió desautenticación.

Deltas PHY CCK_TXEN/TXON/OFDM_TXEN/TXON: autenticación[2,2,0,0], asociación[2,2,1,1], M2[1,1,1,1], M4[1,1,0,0], DISCOVER[1,1,0,0]. Son deltas de ventana, no un conteo inequívoco por trama (pueden incluir autoACK). La evidencia de M3 autenticado acredita avance efectivo más allá de la simple entrega USB de M2; el contraste no demuestra por sí solo causalidad exclusiva del bit SW_MLD.

La captura independiente tiene2342 registros:978 CRC válidos,1363 inválidos,1 truncado y389 beacons válidos. Corrobora gestión, pero no se encontró M2/M4/DISCOVER coincidente ni entre los candidatos con CRC inválido. Esa ausencia no invalida el M3 recibido nativamente ni demuestra ausencia de emisión. El auditor nativo encontró un M1 y un M3, ambos versión2, sin otra trama EAPOL dirigida a la estación.

## Datos cifrados pendientes de autenticar offline

Se encontraron cuatro tramas protegidas FromDS del AP, todas FCS válidas y sin flags CRC/ICV de error: tres de80 bytes y una de380 bytes. Son de grupo. La de380 bytes, en `rx-123.bin`, lleva nuestra MAC en Addr3/origen y es compatible con un DISCOVER reenviado por el AP. Sin verificar MIC/descifrar no se declara que sea la misma carga ni una oferta DHCP.

Se añadió un vector CCMP completo independiente generado con OpenSSL EVP CCM y el PTK público de pruebas, además del vector RFC3610 y los casos anteriores. Coincide con el cifrado del proyecto. Un auditor nuevo reconstruye las claves solo en memoria usando la contraseña local y SNonce/ANonce de la captura; reproduce M2/M4, verifica M3, valida el DISCOVER registrado y autentica las tramas protegidas por orden. Solo escribe metadatos/booleanos, nunca claves o plaintext recibido. No enlaza herramientas USB ni transmite.

Pruebas end-to-end con captura sintética y contraseña pública ficticia: M3/M4, OFFER pairwise, DISCOVER reflejado con GTK, rechazo de contraseña equivocada sin emitir datos. Auditor universal ARM/Intel compilado. Suites C++ y122 pruebas Python pasan. El descifrado de la captura real aún está pendiente porque la contraseña original no se guardó.

Ejecutar como usuario normal, **sin sudo**:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
python3 tools/audit_wpa2_capture.py artifacts/wpa2-stage116/probe/capture artifacts/wpa2-offline-stage117
```

Introducir la contraseña de Especial solo en el prompt local. Compartir la salida completa, sin la contraseña. No hace falta desconectar el adaptador ni repetir WPA2 para este análisis.

## Estado conservado

La retirada H2C y el cierre de callbacks se confirmaron, pero el contexto permanece DIRTY por la fase de datos no confirmada.206 secuencias libres, barrera CLEAN, identidad sin cambio. Postflight físico:210 registros coinciden, TX pausado y concesión original0x01775774 restaurada. No hay lease, interfaz del sistema ni camino cifrado correlacionado declarado todavía.

Evidencia: [observación117 y hashes](wpa2-stage117-observation.json).
