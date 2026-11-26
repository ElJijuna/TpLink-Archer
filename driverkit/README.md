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

## Orden de implementación restante

1. Servicio USB y ciclo Start/Stop/cancelación implementados y enlazados en192; falta contraste físico con la extensión instalada.
2. Adaptar transporte/registros y motor de arranque al entorno DriverKit. El motor IOUSBLib actual usa servicios de macOS y excepciones; no se puede enlazar sin cambios dentro de una DEXT.
3. Backend de criptografía y supplicant persistente: claves en memoria, PN/secuencias por sesión, rekey, desconexión y limpieza verificada. El backend CommonCrypto actual también necesita un límite de plataforma explícito.
4. Pools y colas Ethernet TX/RX, backpressure y completado de propiedad una sola vez; solo publicar link-up después de asociación/claves/contexto válidos. Integrar el codec de esta etapa.
5. Firma/perfil y activación desde la app. El usuario dispone de Apple ID sin membresía Developer; todavía no se acredita ningún perfil ni entitlement DriverKit. La app diagnóstica con firma local sigue siendo independiente de esto.
6. Contraste del sistema: DHCP del stack, DNS, tráfico sostenido IPv4/IPv6, suspensión/reconexión y hardware Intel. No trasladar los leases de ensayos cerrados a la configuración del sistema.

Apple documenta [NetworkingDriverKit](https://developer.apple.com/documentation/NetworkingDriverKit) como interfaz Ethernet y [el proceso de entitlements](https://developer.apple.com/documentation/driverkit/requesting-entitlements-for-driverkit-development) para su instalación. Este proyecto no instala extensiones ni modifica políticas de seguridad mediante los scripts anteriores.

Evidencia: [etapa191](../research/driverkit-stage191-observation.json) y [etapa192](../research/driverkit-stage192-observation.json).
