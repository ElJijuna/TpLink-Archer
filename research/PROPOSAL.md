# Propuesta técnica

## Arquitectura macOS

App SwiftUI → canal de control autorizado → extensión DriverKit → transporte USB → núcleo RTL8922AU.

La app administrará escaneo, asociación y credenciales en Keychain. NetworkingDriverKit publicaría una interfaz Ethernet sobre el enlace inalámbrico; esta integración debe demostrarse antes de construir la interfaz final. No hay promesa de integración con el menú Wi-Fi nativo.

El módulo C++ codifica solicitudes y valida descriptores; el diagnóstico IOUSBLib ya realiza una lectura de registro. Falta implementar transporte USB DriverKit, secuencias de energía, lectura de identidad/chip-cut/efuse, carga de firmware, MAC/BB/RF, colas TX/RX, eventos firmware, gestión 802.11 y autenticación WPA2. NDIS de Windows y mac80211 de Linux no son componentes trasladables directamente a DriverKit.

## Hitos con criterios de salida

1. **Identidad y evidencia (parcial):** INF exacto y familia confirmados; pendiente identificar revisión física y fijar commit de fuentes Linux antes de una adaptación.
2. **Transporte (parcial):** endpoints reales recuperados y lectura de registro 0xFC verificada con IOUSBLib; falta el transporte DriverKit con control de errores, desconexión y cancelación. Ninguna escritura de prueba a registros desconocidos.
3. **Inicialización:** recuperar secuencias de encendido, firmware y datos regulatorios específicos de la placa. Validar chip-cut y procedencia del firmware. El nombre de una cadena de firmware no permite extraer su imagen con seguridad.
4. **Enlace:** escaneo pasivo, asociación 2,4/5 GHz y WPA2; demostrar tráfico bidireccional antes de publicar una interfaz de red. Respetar país, canales y potencia de placa.
5. **Producto macOS:** integrar app y DEXT; verificar entitlements, firma e instalación normal. Probar DHCP, DNS, IPv4/IPv6, suspensión y una hora de transferencia en hardware Intel y ARM.
6. **Linux:** probar primero rtw8922au existente. El PID 0104 no aparece en la tabla consultada; un parche de identificación requiere confirmar compatibilidad y probar hardware antes de upstream. No añadir un ID dinámicamente como sustituto de esa validación.

## Estrategia sin Windows

Contrastar código Linux con llamadas y datos recuperados por REA. Si una diferencia de placa o protocolo no puede resolverse, adquirir una captura Windows de inicialización, escaneo, asociación y tráfico antes de implementar esa parte. No se han observado esas operaciones en esta investigación.

## Primera versión

Cliente WPA2 en 2,4/5 GHz y selección mediante app. Fuera de esta versión: MLO, 6 GHz, WPA3, AP y monitor. Fijar versiones mínimas de macOS después de compilar el transporte DriverKit y comprobar disponibilidad de equipos. El diagnóstico actual usa macOS 12+; no representa todavía la compatibilidad del futuro controlador.

## Referencias

- https://developer.apple.com/documentation/networkingdriverkit (interfaz Ethernet)
- https://developer.apple.com/documentation/driverkit (Intel y Apple Silicon)
- https://developer.apple.com/system-extensions/ (entitlements y distribución)
- https://github.com/torvalds/linux/tree/master/drivers/net/wireless/realtek/rtw89 (referencia técnica; fijar commit antes de reutilizar código)
