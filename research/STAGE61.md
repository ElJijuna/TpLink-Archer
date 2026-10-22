# Etapa61 — recepción asíncrona persistente

Fecha: 2026-10-22. Contraste del mecanismo de recepción, manteniendo el
perfil de contexto, agregación USB y recepción de gestión de la etapa60.
No se recuperan buffers de peticiones fallidas ni se limpian stalls.

## Implementación

MacBulkChannel incorpora un modo opcional, seleccionado por el ejecutor con:

```
--run-owned-source-profile-usb-aggregation-all-management-async NEW_DIRECTORY
```

Mantiene ocho peticiones de20480 bytes sobre0x84, tomando la profundidad y
el tamaño de la fuente Linux fijada c03c1a14507dfaf8b856c2608f9ac216021fc222
(rtw89-usb.c:506-542, rtw89-usb.h:14-15). Añade una petición independiente
sobre0x88 como diagnóstico; esa novena petición no se presenta como parte
del diseño Linux. ReadPipeAsyncTO usa ambos timeouts USB en0.

Un source IOKit en un modo privado de CFRunLoop entrega callbacks en el mismo
hilo. receive espera hasta el plazo del llamador bombeando ese modo, pero
al vencer ese plazo no aborta las peticiones USB. El callback copia solo datos
exitosos, comprueba longitud y límite de cola, y vuelve a enviar su petición.
Errores de transporte, longitud o cola se propagan sin reintentar la operación
fallida. Las colas admiten como máximo1024 transferencias por endpoint.

El contrato de ReadPipeAsyncTO del SDK local IOUSBLib.h:2730-2753 identifica
arg0 como número de bytes completados. Un error no permite decodificar el
buffer. Los callbacks no propagan excepciones por la frontera C.

Al cerrar, el modo stopping impide nuevas peticiones. Se abortan ambos pipes
y se bombea el run loop hasta completar los callbacks pendientes, antes de
liberar source, interfaz y buffers. Si no drenan en cinco segundos, el programa
termina en lugar de liberar memoria todavía referenciada por IOKit. Es una
política del ejecutor experimental; no es recuperación de producción.
La clase no se puede copiar y requiere creación/uso/cierre en el mismo hilo.
Los modos síncronos existentes siguen disponibles para comparar.

## Resultado físico sobre Mac ARM

- Inicio: nueve peticiones pendientes, ocho en0x84 y una en0x88.
- Ocho completados exitosos de36 bytes en0x84: ocho ACK,288 bytes válidos.
- Ningún completado exitoso en0x88. Cero paquetes normales y cero beacons.
- Los nueve H2C del ciclo se enviaron; el comando DMAC no solicita ACK.
- Retiro de contexto reconocido, TX de red pausado, reserva final CLEAN.
- AbortPipe devolvió0 para ambos pipes; llegaron ocho cancelaciones de0x84
  y una de0x88, todas en stopping, con estado-536870165 y longitud0.
- Registro final bulk_async_drained con pending0, salida0 y stderr vacío.

Durante5001 ms, CRC OK PHY aumentó CCK408, OFDM18 y HT2. PLE grupo0 vacío
y cuota6 usada0 en las20 muestras. Los flags RMAC59hex, PHYINFO0ehex,
CMAC40hex y dispatcher c0hex ya estaban al inicio; no aparecieron bits nuevos.
Las muestras no son continuas y no cuentan eventos repetidos sobre bits activos.

La recepción persistente eliminó los abortos periódicos por timeout, pero no
produjo tráfico normal en este perfil. Esto no respalda atribuir el bloqueo
únicamente al uso de lecturas síncronas de10 ms. No descarta otros fallos del
transporte ni determina dónde se retienen o descartan las tramas. El próximo
contraste debe revisar la configuración efectiva de entrega RMAC/PLE/USB
contra la secuencia fuente del chip, preservando el límite de transmisión.

## Validación y límites

49 suites C++ y55 tests Python existentes pasan; compilación arm64/x86_64.
La prueba física verifica completados, resubmisión y cancelación normal con el
adaptador conectado. No cubre desconexión física, agotamiento de memoria,
cola saturada ni callbacks que no drenen. Intel solo se compiló. No hay aún
interfaz Wi-Fi funcional y el avance ponderado sigue en40 %.

El binario se conservó antes de la prueba. Después solo se añadieron comentarios
al header; el build final se conserva por separado. Hashes, completados y
resumen PHY: [async-receive-stage61-observation.json](async-receive-stage61-observation.json).
