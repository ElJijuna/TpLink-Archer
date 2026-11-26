# Etapa183 — Archer Control para macOS

Se construyó la primera aplicación nativa de gestión y diagnóstico, SwiftUI/Swift Charts, macOS14+, universal ARM/Intel y firma local ad hoc. Bundle: `build/Archer Control.app`. Uso y límites: [desktop/README.md](../desktop/README.md).

Incluye enumeración USB, contexto guardado, catálogo de capturas, redes observadas, gráficas de tráfico/RSSI, explorador de paquetes con hexadecimal y filtros, exportación CSV/PCAP, integridad SHA256 y resultados del ejecutor. Las acciones de ensayo y expulsión abren los helpers existentes en Terminal tras una comprobación nueva. La contraseña permanece en los prompts locales ocultos.

## Verificación

31 comprobaciones del parser, exportación y admisión pasaron. La captura179 produjo2638 registros decodificados,10 redes válidas y1056 fallos CRC calculados; el registro nativo acredita1 trama pairwise. En la UI de Apple Silicon se revisaron las seis vistas, detalle hexadecimal, filtro sin resultados y borrado de selección, exportación de506 filas filtradas, importación PCAP y conservación de la selección al actualizar. Intel se compiló, pero no se probó físicamente. Se añadió indicador de carga inicial.

No se ejecutó ningún ensayo de radio durante este desarrollo ni se consumió el marcador182. No se modificaron los helpers de radio. El contexto almacenado permanece CLEAN con206 secuencias; la enumeración USB de la aplicación no sustituye la admisión física del backend.

## Pendiente

La aplicación es utilizable para gestión diagnóstica y capturas, pero todavía no conecta macOS a Internet por el Archer. La etapa182 ICMP sigue pendiente de ejecución local. Faltan integración de interfaz/IP, ciclo de vida/reconexión y estabilidad del controlador. Estación macOS, monitor general del Archer y AP permanecen deshabilitados. Las gráficas proceden de archivos, no de un escaneo continuo. El alcance ponderado de investigación sigue en66/100, sin estimación temporal.

Evidencia: [desktop-stage183-observation.json](desktop-stage183-observation.json). Los PCAP originales y reservas se conservan.
