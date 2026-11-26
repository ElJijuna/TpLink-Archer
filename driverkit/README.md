# Integración de red macOS — en desarrollo

El ensayo189 y su auditoría190 comprobaron WPA2, DHCP, ARP y un Echo IP pairwise al gateway. Eso acredita el transporte experimental; todavía no existe un servicio de red persistente ni una DEXT instalable.

## Primer componente implementado

`include/archer/ethernet_bridge.hpp` convierte Ethernet II del sistema a LLC/SNAP RFC1042 y reconstruye Ethernet desde un MSDU ya autenticado. No asigna memoria, no usa excepciones y recibe buffers con capacidad explícita. El MTU máximo es1500. Admite IPv4/ARP/IPv6 como tipos de paquete; esto no acredita IPv6 por radio. Rechaza VLAN, EAPOL del sistema, tipos no implementados, remitentes ajenos a la estación, reflejos locales, buffers cortos y solapados. Los errores no escriben parcialmente el buffer de salida.

La frontera RX **no autentica**: el llamador debe completar primero CCMP MIC/PN y validar BSSID, dirección y receptor. EAPOL permanece dentro del supplicant. Este componente todavía no está conectado a USB ni a las colas de macOS.

```sh
./driverkit/test_bridge.sh   # ASan/UBSan + compilación universal del test
./driverkit/check_sdk.sh     # compila frontera y cabeceras del SDK para ARM/Intel
```

Verificado con DriverKit SDK27.2 de Xcode, targets de compilación DriverKit24.0. La comprobación SDK genera objetos; **no enlaza, firma, instala ni ejecuta una extensión**. No establece la versión mínima del futuro producto.

## Orden de implementación restante

1. Servicio `IOUserNetworkEthernet` con proveedor USB exacto37ad:0104 y ciclo Start/Stop/cancelación. El instalador0bda:1a2b debe quedar fuera de ese matching.
2. Adaptar transporte/registros y motor de arranque al entorno DriverKit. El motor IOUSBLib actual usa servicios de macOS y excepciones; no se puede enlazar sin cambios dentro de una DEXT.
3. Backend de criptografía y supplicant persistente: claves en memoria, PN/secuencias por sesión, rekey, desconexión y limpieza verificada. El backend CommonCrypto actual también necesita un límite de plataforma explícito.
4. Pools y colas Ethernet TX/RX, backpressure y completado de propiedad una sola vez; solo publicar link-up después de asociación/claves/contexto válidos. Integrar el codec de esta etapa.
5. Firma/perfil y activación desde la app. El usuario dispone de Apple ID sin membresía Developer; todavía no se acredita ningún perfil ni entitlement DriverKit. La app diagnóstica con firma local sigue siendo independiente de esto.
6. Contraste del sistema: DHCP del stack, DNS, tráfico sostenido IPv4/IPv6, suspensión/reconexión y hardware Intel. No trasladar los leases de ensayos cerrados a la configuración del sistema.

Apple documenta [NetworkingDriverKit](https://developer.apple.com/documentation/NetworkingDriverKit) como interfaz Ethernet y [el proceso de entitlements](https://developer.apple.com/documentation/driverkit/requesting-entitlements-for-driverkit-development) para su instalación. Este proyecto no instala extensiones ni modifica políticas de seguridad mediante los scripts anteriores.

Evidencia: [etapa191](../research/driverkit-stage191-observation.json).
