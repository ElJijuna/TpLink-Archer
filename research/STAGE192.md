# Etapa192 — servicio DriverKit USB: build completo, sin instalación

2026-11-26. Se implementó ArcherUSB, subclase IOUserNetworkEthernet con proveedor IOUSBHostInterface. Personalidad37ad:0104/interfaz0/config1/FF:FF:FF; verificación adicional del descriptor y los nueve pipes bulk512 del perfil USB2. El instalador y perfiles no medidos se rechazan.

Start/Stop comparten un núcleo sin excepciones ni asignaciones con las pruebas. Se retiene/abre el proveedor y se valida antes de adquirir pipes. Ante fallo parcial, Stop deja de admitir trabajo, aborta control y pipes, cierra la interfaz y libera referencias. Los errores de abort/close no omiten la limpieza restante; se conserva el primer error. Stop es idempotente y no permite reinicio de una instancia detenida. Las llamadas se serializan en la cola predeterminada; no hay callbacks de transferencias asíncronas propios ni buffers de I/O en esta etapa. El cierre real de solicitudes pendientes debe contrastarse cuando exista transporte y una extensión instalada.

IIG generado y ejecutable enlazado con SDK DriverKit27.2, target24.0, en ARM e Intel. Bundle universal sin instalación: `build/driverkit/service/ArcherUSB.dext`. Comprobación Mach-O confirma plataforma10, ambas arquitecturas y dependencias DriverKit/USBDriverKit/NetworkingDriverKit. Plist conforme a la personalidad Ethernet USB de Apple. La plantilla de entitlements no se aplica ni concede derechos de instalación.

15 escenarios de fallo/Start/Stop, callback de cancelación reentrante, cierre repetido, descriptor físico, truncaciones y mutaciones rechazadas pasaron ASan/UBSan. Las pruebas ejercitan el mismo núcleo que usa la clase DriverKit, con backend simulado; no ejecutan las RPC dentro del sistema. Sin I/O USB o radio nuevos. No se instala DEXT, no se crea interfaz, no se carga firmware, no se publica link-up.

El punto1 está implementado y verificado en compilación/pruebas; su prueba de hardware instalada sigue pendiente. El punto2 es adaptar transporte/control de registros y luego motor de arranque, sin reutilizar directamente IOUSBLib ni las excepciones del diagnóstico. La sesión WPA2 persistente y las colas Ethernet siguen pendientes.

Referencias oficiales: [IOUserNetworkEthernet](https://developer.apple.com/documentation/networkingdriverkit/iousernetworkethernet), [IOUSBHostInterface](https://developer.apple.com/documentation/usbdriverkit/iousbhostinterface), [Close](https://developer.apple.com/documentation/usbdriverkit/iousbhostinterface/close) y [Abort](https://developer.apple.com/documentation/usbdriverkit/iousbhostpipe/abort).
