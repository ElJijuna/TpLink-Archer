# Avance hacia un controlador utilizable

Actualizado el 20 de octubre de 2026. Los diagnósticos funcionan en el Mac ARM
actual. Compilar para Intel no demuestra funcionamiento en hardware Intel.
No hay todavía una interfaz de red creada por este proyecto ni conexión Wi-Fi.

| Bloque | Estado real | Criterio de salida pendiente |
| --- | --- | --- |
| Identidad y diagnóstico USB | Validado en el equipo actual | Mantener regresiones y probar otro equipo/reconexión |
| Carga principal/BBMCU y respuesta RF | Hito experimental validado en el Mac ARM actual | Completar inicialización/calibración sostenida y recuperación automática; probar Intel |
| Transporte de datos y gestión Wi-Fi | Pendiente | TX/RX de datos, eventos, escaneo y asociación |
| Conexión utilizable | Pendiente | Autenticación WPA2 y tráfico bidireccional con DHCP/DNS |
| Integración y distribución macOS | Pendiente | Transporte DriverKit, interfaz de red, instalación/firma, Intel/ARM, suspensión y estabilidad |
| Linux | Posterior | Probar soporte existente y validar el ID/modelo exacto en hardware |

Estos bloques tienen esfuerzos diferentes: contar archivos, pruebas o etapas
de investigación no da un porcentaje fiable de producto terminado. Sigue
siendo trabajo de desarrollo e investigación considerable, no un último ajuste.
No hay una fecha fiable hasta validar primero arranque RF y después un enlace.

## Qué ya funciona

- Identificación USB 37ad:0104, revisión digital 1, AID 0x6933 y RFE 1.
- Lectura de registros y efuse, con restauración de controles temporales.
- Consulta PHY_CAP por buzón y ACKs de comandos diagnósticos por bulk USB.
- Acceso a registros BB dentro de una ventana temporal de diagnóstico.
- Decodificación offline de firmware, tablas BB/RF, calibración y paquetes.
- Fuentes y firmware de referencia fijados por commit y hash.

## Hito validado más reciente

Carga RAM principal0.35.41.0 + BBMCU0 0.1.0.0 aceptada por los indicadores
de la ROM, consulta PHY_CAP posterior y ocho lecturas RF completadas. Se usó
un perfil USB experimental que no escribe el bloque DCPU inaccesible. No
es una validación de todas las funciones BBMCU ni de calibración. Evidencia:
[firmware-rf-stage29-observation.json](firmware-rf-stage29-observation.json).

## Investigación anterior

La lectura de seguridad efuse 0x1580–0x1583 devolvió `ff ff ff ff`. Según el
código Linux fijado, el selector 0xffff toma la ruta predeterminada. Esto no
verifica criptográficamente las imágenes ni garantiza aceptación por la ROM.

La preparación offline ahora genera 193 paquetes para firmware principal
0.35.41.0 y 19 para BBMCU0 0.1.0.0. La cabecera usa H2C; los datos usan tipo
FWDL, sin cabecera H2C por fragmento. El generador sigue siendo offline. Se añadió un ejecutor nativo experimental:
el primer intento inició el reset de CPU y se detuvo al leer 0x888=deadbeef,
antes de enviar firmware. El acceso ahora se comprueba antes del reset.
La carga principal/BBMCU y la respuesta RF siguen sin validar.

## Próximo hito comprobable

Consolidar el arranque diagnóstico como inicialización sostenida: completar
MAC/DLE, BB gain/postinit y radio/calibración, junto con recuperación por
fallo o desconexión. Después validar escaneo y asociación. La carga y la
respuesta RF ya tienen evidencia física; no deben confundirse con un enlace
Wi-Fi, una radio calibrada ni una interfaz de red lista para uso.
