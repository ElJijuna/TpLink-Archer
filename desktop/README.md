# Archer Control — aplicación macOS

Primera versión local de escritorio (SwiftUI + Swift Charts), macOS14 o posterior. La UI se verificó en Apple Silicon; Intel solo se compiló, sin prueba física todavía. Compila para Apple Silicon e Intel sin dependencias externas. El controlador de red todavía está en investigación: esta app no crea una interfaz ni permite navegar por el Archer.

## Abrir y construir

Desde la raíz del proyecto:

```sh
./desktop/test.sh
./desktop/build.sh
open "build/Archer Control.app"
```

Si macOS no muestra las capturas al primer inicio, pulsa **Cambiar proyecto** y selecciona la carpeta `Tp-link-Archer`; después aparecerán los datos locales.

La app está firmada localmente con una firma ad hoc. No es un instalador DriverKit ni una distribución notarizada. El proyecto por defecto se incorpora al bundle durante la compilación; se puede cambiar desde la barra lateral. También acepta `--project /ruta/al/proyecto` al abrir el ejecutable.

## Qué funciona

- Enumeración USB actual del Archer Wi-Fi37ad:0104 y del instalador0bda:1a2b, sin acceder a registros ni transmitir al abrir la app.
- Contexto guardado, época y secuencias restantes. Se distinguen del estado físico USB y de los resultados históricos.
- Catálogo de capturas del coordinador, lectura de manifiestos y registro nativo, y comparación SHA256 del PCAP/log con su manifiesto.
- Gráficas reales de actividad por tipo de trama y RSSI radiotap; redes observadas a partir de beacons/probe responses. Son datos de la captura seleccionada, no un escaneo en vivo ni un medidor de Internet.
- Explorador 802.11 con filtros por tipo, texto y estación; detalle hexadecimal; CSV de la vista y copia del PCAP completo. La app no descifra payloads protegidos ni solicita claves.
- Resultados WPA2/DHCP/ARP/ICMP y reportes TX del ejecutor cuando existen, con su procedencia. El registro nativo prueba la autenticación; el bit Protected del PCAP por sí solo no la prueba.
- Apertura en Terminal del ensayo ICMP182 preparado, solo si coinciden USB Wi-Fi, CLEAN/fence, época, ledger, hashes y marcador ausente. El backend vuelve a ejecutar su admisión completa. Se usa una ruta de salida nueva y no se borran reservas.
- Expulsión del instalador a través del helper existente en Terminal, con verificación de identidad por el helper.

## Modos y límites

Inspección y análisis PCAP son locales. El ensayo guiado es una acción explícita que puede desconectar en0 y solicitar un lease temporal; sus límites, RELEASE y cierre pertenecen al coordinador existente. Las contraseñas se introducen únicamente en Terminal, nunca en la app. macOS puede pedir autorización de Automatización para abrir comandos en Terminal; si no se concede, se puede copiar el comando y ejecutarlo manualmente.

Estación macOS, monitor general del Archer y punto de acceso aparecen como pendientes y no activan la radio. Los PCAP independientes actuales fueron capturados con en0; eso no demuestra un modo monitor general en el Archer. No se implementó Keychain, DriverKit, reconexión automática ni controles genéricos de potencia/canal.

El parser admite PCAP clásico radiotap (endianness y micro/nanosegundos), FCS cuando el formato lo permite, registros parciales de archivos en escritura y un máximo de25000 registros por vista. Conserva los originales y avisa de límites; PCAPNG y archivos mayores de64MiB no están admitidos todavía. Los datos MAC/IP sin cifrar de una captura importada siguen siendo visibles en sus bytes; las claves de los ensayos no se guardan.

## Arquitectura

`ArcherControl/Core.swift` contiene parser, modelos, exportación y admisión de UI. `AppModel.swift` carga archivos en segundo plano, consulta ioreg y realiza las acciones explícitas de Terminal/Finder. `Views.swift` implementa las seis vistas. `workflows.json` describe el único flujo de radio preparado; una nueva etapa requiere actualizar esta definición y su manifiesto real, no solo cambiar el número en la interfaz.

La integración de red posterior necesita el servicio/controlador y su ciclo de vida, firma y permisos correspondientes. [DriverKit y extensiones del sistema](https://developer.apple.com/system-extensions/) describe el modelo de despliegue. No se cambia SIP ni se instala ninguna extensión con esta versión.
