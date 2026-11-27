# Etapa196 — ventanas WLAN/RF y restauración sin excepciones

2026-11-27. DriverPowerWindows porta la ventana WLAN, lectura de identidad analógica y secuencia de potencia RF a acciones serializadas. El chip/cut se rechaza antes de comandos SIO si no coincide. En la ruta de firmware, las comprobaciones cold BB/reset se realizan antes de configurar RF. La identidad analógica y las bases XTAL siguen vinculadas a6933/CV1.

El diario almacena máscaras y valores originales antes de escrituras potencialmente inciertas. Restaura en orden inverso, preserva campos ajenos y verifica. No intenta restituir CPU, imágenes o controles modificados por el cuerpo de firmware. Los fallos de cleanup se registran con primer error y bitmap; no se detiene el resto del cleanup ni se declara restauración completa si falla una entrada.

close_rf deja WLAN abierta después de restituir RF; finish_wlan cierra la ventana WLAN después de que el futuro coordinador termine controles y drenaje. El modo de cierre conjunto conserva la equivalencia con los wrappers anteriores. La cancelación de una petición de control pendiente no emite restauraciones hasta su confirmación terminal. El abandono o el límite de restauración conservan la marca de campos no confirmados.

Pasaron ASan/UBSan y la comparación de trazas válidas con la implementación previa: WLAN original activa/inactiva, identidad, bases, apertura y cleanup.1576 inyecciones de fallo incluyen operaciones con efecto incierto. Cobertura adicional: SIO con dos polls ocupados, timeout de seis polls, timeout de activación WLAN, selector incorrecto, errores múltiples durante restauración, cancelación, desconexión, presupuesto vencido y no rollback de firmware. Una prueba fría reproduce también la comprobación BB/reset previa de la ruta anterior. Los tests son simulación; no son mediciones nuevas del USB.

Las entradas de ventanas están compiladas/enlazadas con DriverKit27.2/target24.0 para ARM e Intel. La DEXT mantiene matching37ad:0104 y no expone nuevas RPC de control. Start no llama al motor ni abre ventanas. Stop inesperado abandona la observación sin prometer que restauró hardware. No se instaló la DEXT, no se descargó firmware y no hubo solicitudes USB/Wi-Fi.

Siguiente integración: unir ventanas, descarga, tablas posteriores, restauración parcial, verificación y drenaje bajo un coordinador con propiedad/fence y dispatcher/timers. La ventana aislada no concede permisos de arranque ni sustituye el ledger. Evidencia: [driverkit-stage196-observation.json](driverkit-stage196-observation.json).
