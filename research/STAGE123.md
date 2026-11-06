# Etapa123 — broadcast autenticado, sin OFFER; ARP pendiente de clasificar

2026-11-06. El intento122 se ejecutó una vez. Se verificaron hashes del ejecutable conservado, stdout/stderr y PCAP frente a sus manifiestos. Especial aceptó autenticación y asociación/AID1; M3 pasó autenticación criptográfica. Se entregaron M4 y DISCOVER cifrado por USB09.

La instrumentación nativa autenticó seis tramas CCMP de grupo: una copia exacta del DISCOVER esperado con flags0x8000 y origen propio autenticado, más cinco ARP. Cero rechazos, cero tramas pairwise y cero OFFER coincidentes. La espera de tres segundos terminó por timeout. No se envió la fase5 de desautenticación, conservando la política de entrega pendiente y contexto DIRTY. El cambio a broadcast no bastó para obtener OFFER en esta ventana; no demuestra que la causa sea exclusivamente temporización, servidor o recepción unicast.

El usuario confirma que otro dispositivo conectado a Especial obtiene IP automáticamente. Esto acredita por su informe que existe DHCP operativo para ese cliente, pero no descarta políticas o diferencias específicas de nuestro ensayo. La máscara de filtro0x101 incluye datos normales y QoS, subtipos0 y8; la fuente retenida `rtw89-reg.h` define un bit por subtipo dentro de GENMASK(15,0). No se encontró una exclusión de QoS en esa máscara y no se cambió el filtro.

## Captura y estado conservado

PCAP externo:3125 registros,1166 CRC válidos,1955 inválidos,4 truncados y402 beacons válidos de Especial. Hay nueve registros válidos de nuestra MAC; esta cuenta por sí sola no corrobora el intercambio de datos cifrado, cuya evidencia es la autenticación nativa. `capture_exit=0`; la salida1 del helper corresponde al fallo al restaurar el canal original de en0. Reconectar el Wi-Fi del Mac si no se restauró automáticamente.

Los ocho pasos ACK del contexto llegaron y los callbacks se drenaron. El postflight físico posterior confirmó210 registros, cero diferencias, TX pausado y concesión original0x01775774 restaurada. Contexto DIRTY, barrera de época CLEAN y206 secuencias libres; UUID7184d1e9-6f13-45a3-981a-635cadffdb33. Ninguna transmisión nueva en esta etapa de análisis. No repetir122 ni liberar reservas manualmente.

## Siguiente comprobación sin radio

Se amplió el auditor offline para clasificar ARP Ethernet/IPv4 de28 bytes: operación, si las direcciones IPv4 son cero y si las MAC coinciden con AP/estación. Solo imprime números y booleanos de metadatos, sin direcciones IP, payload ni claves. Las pruebas sintéticas cubren estos campos junto con las variantes DISCOVER normales/broadcast y contraseña equivocada. Suites C++ y123 pruebas Python pasan; auditor ARM/Intel compilado. Falta ejecutar esta auditoría sobre la captura real: la contraseña no se conservó.

Ejecutar como usuario normal, **sin sudo**:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
python3 tools/audit_wpa2_capture.py artifacts/wpa2-stage122/probe/capture artifacts/wpa2-offline-stage123
```

Introducir la contraseña de Especial únicamente en el prompt local oculto. Compartir salida completa sin contraseña. No requiere desconectar el Archer, accede solo a archivos y no transmite. Los ARP podrían aportar una pista, pero todavía no se les atribuye relación con DHCP.

Sin OFFER, lease, recepción unicast física o interfaz del sistema nuevos. Avance ponderado conservado en60 %.

[Observación y hashes](wpa2-stage123-observation.json).
