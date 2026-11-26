# Integración de red macOS — en desarrollo

El ensayo189 y su auditoría190 comprobaron WPA2, DHCP, ARP y un Echo IP pairwise al gateway. Eso acredita el transporte experimental; todavía no existe una conexión de red persistente utilizable. La etapa192 ya construye el servicio USB como bundle DEXT universal sin instalarlo.

## Primer componente implementado

`include/archer/ethernet_bridge.hpp` convierte Ethernet II del sistema a LLC/SNAP RFC1042 y reconstruye Ethernet desde un MSDU ya autenticado. No asigna memoria, no usa excepciones y recibe buffers con capacidad explícita. El MTU máximo es1500. Admite IPv4/ARP/IPv6 como tipos de paquete; esto no acredita IPv6 por radio. Rechaza VLAN, EAPOL del sistema, tipos no implementados, remitentes ajenos a la estación, reflejos locales, buffers cortos y solapados. Los errores no escriben parcialmente el buffer de salida.

La frontera RX **no autentica**: el llamador debe completar primero CCMP MIC/PN y validar BSSID, dirección y receptor. EAPOL permanece dentro del supplicant. Este componente todavía no está conectado a USB ni a las colas de macOS.

```sh
./driverkit/test_bridge.sh   # ASan/UBSan + compilación universal del test
./driverkit/check_sdk.sh     # compila frontera y cabeceras del SDK para ARM/Intel
```

Verificado con DriverKit SDK27.2 de Xcode, targets de compilación DriverKit24.0. La comprobación SDK genera objetos; **no enlaza, firma, instala ni ejecuta una extensión**. No establece la versión mínima del futuro producto.

## Servicio USB implementado — etapa192

`ArcherUSB` deriva de `IOUserNetworkEthernet`, con proveedor `IOUSBHostInterface`. El matching y la comprobación en Start limitan el servicio a37ad:0104, configuración1, interfaz0, alt0 y el perfil USB2 físico conservado en fixtures. No acepta el instalador0bda:1a2b ni perfiles USB3 todavía.

Start retiene/abre el proveedor, verifica descriptores y adquiere los nueve pipes. Ante fallo parcial limpia los recursos adquiridos. Stop cierra primero la admisión, aborta solicitudes de control y cada pipe, cierra la interfaz y libera los pipes/proveedor. Las cancelaciones de pipes son síncronas. Stop repetido/reentrante no vuelve a liberar recursos. El núcleo de ciclo de vida se ejecuta de forma serial en la cola predeterminada del servicio; no inicia transferencias asíncronas en esta etapa. `ready` solo significa recursos USB adquiridos, nunca asociación ni link-up.

No se llama a RegisterEthernetInterface/ReportLinkStatus, no se carga firmware y habilitar la interfaz devuelve NotReady. Los callbacks obligatorios antiguos del ABI de NetworkingDriverKit están implementados con capacidades limitadas; la compilación suprime solo sus avisos de deprecación. Esto no sustituye la implementación de colas y supplicant.

```sh
./driverkit/test_service.sh
./driverkit/build.sh
python3 driverkit/verify_bundle.py
```

Salida: `build/driverkit/service/ArcherUSB.dext`. El build genera IIG, compila/enlaza para ARM/Intel, crea un ejecutable universal y un plist con matching exacto. No instala ni aplica entitlements. `ArcherUSB/Entitlements.example.plist` es una plantilla de las capacidades que deberán autorizarse con el perfil apropiado, no una autorización concedida.

Pasaron15 escenarios de Start/Stop con fallos inyectados y sanitizadores, incluyendo fallo en cada adquisición, cancelación reentrante y cierre con errores. Se reprodujo el descriptor físico y se comprobaron identidad/plataforma DriverKit/frameworks de ambas arquitecturas. **Start/Stop no se ejecutaron dentro de una extensión instalada**: esa prueba física queda pendiente de firma y activación. No se atribuye funcionamiento de hardware a estas pruebas simuladas.

## Transporte DriverKit implementado — etapa193

El servicio incorpora APIs **LOCALONLY**, utilizables por el futuro motor dentro de la DEXT: `ReadRegister`, `WriteRegister`, `SubmitBulk`, `CopyBulkResult`, `ReleaseBulk` y `GetTransportStatus`. El código generado IIG confirma que no son RPC de acceso externo. Deben invocarse en la cola serial Default; el transporte rechaza otras colas. No existe user client ni invocación desde la app todavía.

- Registros: solicitud vendor05, dirección24 bits repartida entre value/index y anchura1/2/4, como en IOUSBLib. Timeout200ms, contador exacto y little-endian. Las lecturas fallidas/cortas no publican un valor. La primitiva codifica el protocolo; la política de qué registros modificar corresponde al motor de inicialización.
- Bulk: doce slots, buffers DMA preasignados de16KiB, dos buffers de control de4 bytes y una acción por solicitud. Se permiten solo los nueve endpoints verificados. Timeout explícito1–2000ms, sin rearmado ni reenvío automático. El máximo es192KiB+8 de capacidad útil de buffers, además del overhead del SDK.
- Completado: un ticket no reutilizado y una época de instancia dentro del proceso. Cada acción conserva un token inmutable; callbacks duplicados/de otra época o slot se ignoran. RX corto es válido; OUT corto, exceso de longitud o error no se publican como datos correctos. Un consumidor copia el resultado y lo libera explícitamente. La época de callbacks no sustituye el ledger H2C ni valida la antigüedad del payload firmware; eso sigue perteneciendo al motor.
- Cancelación: un slot no se recicla hasta que el handler de `OSAction::Cancel` confirma que sus callbacks finalizaron. Stop cierra admisión, aborta/cierra USB y difiere el Stop heredado hasta terminar los controles y cancelar todas las acciones. Si falla la confirmación de Cancel o Close, conserva los recursos en cuarentena en lugar de forzar su liberación. `GetTransportStatus` expone el fallo al motor futuro; no hay recuperación automática.

```sh
./driverkit/test_transport.sh
./driverkit/test_service.sh
./driverkit/build.sh
python3 driverkit/verify_bundle.py
```

Pruebas ASan/UBSan: codificación diferencial con el transporte anterior, lecturas parciales/error, backpressure, identificadores agotados, callbacks antiguos/duplicados, cuarentena sin confirmación y reciclado tras Cancel. El bundle enlaza para ARM/Intel y mantiene plataforma/matching exactos. El binario probado se conserva en `artifacts/driverkit-stage193-ArcherUSB`.

**Sin contraste físico dentro de una DEXT instalada.** Construir o iniciar este servicio no envía solicitudes de registros/bulk automáticamente ni carga firmware; el llamador del motor debe hacerlo explícitamente en la próxima etapa. No se ha portado todavía ese motor, su política de reservas, los decodificadores/eventos ni el backend criptográfico persistente. No se crea interfaz Ethernet.

## Primera conexión al motor: observación de arranque — etapa194

`ReadBootControls` lee FC/F0/1E0/84/D0/842C mediante el nuevo transporte, en una llamada local serializada. Solo publica el conjunto si todas las lecturas terminan correctamente y el transporte sigue vigente. Rechaza los valores de registro inaccesible conocidos. Devuelve observaciones de chip/cut, controles fríos, controles de firmware en ejecución o estado desconocido. **Ninguna clasificación acredita identidad/liveness del firmware ni autoriza reset, escritura o descarga.** Todavía faltan propiedad exclusiva, validación de imágenes, ventanas WLAN/RF, secuencias, confirmaciones y drenaje del motor completo.

```sh
./driverkit/test_boot_controls.sh
```

La prueba usa los primeros controles leídos en la captura fría188 y un caso de ejecución sintético separado. Verifica sentinelas, cut incorrecto y estados ROM de rechazo. El método no se llama automáticamente desde Start y no se ejecutó sobre hardware en esta etapa. Sigue siendo LOCALONLY y el bundle universal compila/enlaza con él.

## Núcleo de descarga de firmware portado — etapa195

`DriverFirmwareEngine` porta la secuencia de `boot_firmware(..., usb_without_dcpu=true)` a una máquina de acciones sin excepciones, asignaciones dinámicas ni esperas bloqueantes dentro del núcleo. Produce lecturas, escrituras, bulk OUT07 y pausas de10ms; el coordinador debe completarlas explícitamente. Una sola acción pendiente, época/ticket, plazo global30s, hasta100 polls por condición, rechazo ROM4–7 y comprobación exacta de bytes. Una acción fallida/corta no se vuelve a emitir. Una instancia no permite un segundo inicio. Registra mutación y tentativa de reset antes de entregar la escritura, sin afirmar rollback.

`bind_firmware.py` exige el SHA256 del archivo original ya utilizado por el proyecto y usa el generador anterior para producir dos imágenes inmutables con212 paquetes. No modifica seguridad, firmas ni contenido del firmware. La comparación con el header anterior verifica todos los bytes/orden. Un archivo alterado se rechaza antes de escribir archivos generados. El build incorpora las imágenes y enlaza explícitamente las entradas C++ del núcleo para ambas arquitecturas. `GetBoundFirmwarePlan` es LOCALONLY y solo consulta metadatos; no inicia el motor.

```sh
./driverkit/test_firmware.sh
python3 -m unittest discover -s tests -p test_driver_firmware_bind.py
./driverkit/build.sh
python3 driverkit/verify_bundle.py
```

Las pruebas ASan/UBSan comparan todas las operaciones con la rutina anterior sobre un backend simulado, incluyendo los212 paquetes reales vinculados. Pasaron101 puntos de fallo inyectado, rechazo ROM, timeout, cancelación, reloj/plazo, imágenes inválidas y transferencia corta en el último paquete. Dos pruebas Python verifican binding y rechazo por hash. Esto **no representa confirmaciones ROM nuevas obtenidas del hardware**.

**El arranque completo aún no está habilitado.** Faltan las ventanas WLAN/RF con sus restauraciones, la política de propiedad/fence, tablas posteriores, dispatcher/timers que conecten las acciones al transporte y drenaje final. Los controles fríos de194 no conceden esos permisos. Start no llama al núcleo y no existe una API que permita comenzar un reset desde la app. El bundle sigue sin instalarse; no hubo solicitudes USB ni cargas reales en esta etapa.

## Orden de implementación restante

1. Servicio USB y ciclo Start/Stop/cancelación implementados y enlazados en192; falta contraste físico con la extensión instalada.
2. Transporte adaptado en193 y núcleo de descarga portado/enlazado en195. Faltan ventanas WLAN/RF, propiedad/fence, dispatcher/timers, tablas posteriores y drenaje antes de habilitar el arranque completo.
3. Backend de criptografía y supplicant persistente: claves en memoria, PN/secuencias por sesión, rekey, desconexión y limpieza verificada. El backend CommonCrypto actual también necesita un límite de plataforma explícito.
4. Pools y colas Ethernet TX/RX, backpressure y completado de propiedad una sola vez; solo publicar link-up después de asociación/claves/contexto válidos. Integrar el codec de esta etapa.
5. Firma/perfil y activación desde la app. El usuario dispone de Apple ID sin membresía Developer; todavía no se acredita ningún perfil ni entitlement DriverKit. La app diagnóstica con firma local sigue siendo independiente de esto.
6. Contraste del sistema: DHCP del stack, DNS, tráfico sostenido IPv4/IPv6, suspensión/reconexión y hardware Intel. No trasladar los leases de ensayos cerrados a la configuración del sistema.

Apple documenta [NetworkingDriverKit](https://developer.apple.com/documentation/NetworkingDriverKit) como interfaz Ethernet y [el proceso de entitlements](https://developer.apple.com/documentation/driverkit/requesting-entitlements-for-driverkit-development) para su instalación. Este proyecto no instala extensiones ni modifica políticas de seguridad mediante los scripts anteriores.

Evidencia: [etapa191](../research/driverkit-stage191-observation.json) y [etapa192](../research/driverkit-stage192-observation.json) y [etapa193](../research/driverkit-stage193-observation.json) y [etapa194](../research/driverkit-stage194-observation.json) y [etapa195](../research/driverkit-stage195-observation.json).
