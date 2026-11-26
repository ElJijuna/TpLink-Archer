# Etapa193 — transporte USB DriverKit compilado, sin instalación

2026-11-26. Se incorporó transporte local al servicio ArcherUSB: control de registros vendor05/address24/width1,2,4 con timeout200ms y longitud exacta; doce buffers DMA de16KiB y dos de control4 bytes; AsyncIO en los endpoints verificados con timeout explícito1–2000ms.

Cada solicitud consume un ticket sin reutilización y una época de instancia dentro del proceso. Los callbacks validan acción/época/ticket/slot, no procesan datos firmware y no rearman solicitudes. RX corto se conserva con su tamaño real; OUT corto, exceso o error no se convierten en éxito. La liberación del resultado inicia Cancel y conserva el slot hasta la confirmación SDK. No hay reenvío de OUT incierto.

El Stop de192 fue ampliado: primero cierra admisión, aborta y cierra la interfaz, conserva descriptores/acciones hasta los handlers Cancel y operaciones de control terminadas, y después llama al Stop heredado y libera el proveedor. Se serializa en Default, incluyendo callbacks que lleguen por otra cola. Fallos de Cancel/Close dejan recursos retenidos y fallo consultable, sin liberación forzada ni reintento automático. La época de callbacks no acredita frescura del contenido C2H: el motor aún debe validar sus secuencias/ledger y drenaje.

La implementación usa el estado sin excepciones probado en host. ASan/UBSan pasó el contraste de encoding con register_request anterior, límites de direcciones/anchuras/timeouts/capacidades, lecturas cortas sin datos obsoletos, pool lleno, short OUT, error/overrun, callbacks duplicados y de otra sesión, agotamiento de ticket y reciclado únicamente después de Cancel. Los15 escenarios previos de ciclo de vida también pasan.

IIG/compilación/enlace ARM e Intel con SDK DriverKit27.2 y target24.0 completados. La verificación del bundle confirma plataforma DriverKit, frameworks, matching exacto y que las seis APIs de transporte son LOCALONLY, sin RPC pública de registros. El binario de esta etapa está archivado fuera de build/ para conservarlo al reconstruir.

No se instaló la DEXT ni se ejecutaron sus solicitudes USB sobre hardware; tampoco se cargó firmware, se transmitió Wi-Fi o se creó una interfaz. Start solo adquiere recursos; las operaciones son explícitas. Falta conectar el motor de arranque, las reservas de firmware y la recepción C2H; luego supplicant persistente y colas Ethernet. La firma/activación normal y su prueba física siguen pendientes.

Referencias oficiales: [DeviceRequest](https://developer.apple.com/documentation/usbdriverkit/iousbhostinterface/devicerequest), [DriverKit y callbacks](https://developer.apple.com/documentation/driverkit/creating-a-driver-using-the-driverkit-sdk) y [OSAction](https://developer.apple.com/documentation/driverkit/osaction).

Uso y límites: [driverkit/README.md](../driverkit/README.md). Evidencia: [driverkit-stage193-observation.json](driverkit-stage193-observation.json).
