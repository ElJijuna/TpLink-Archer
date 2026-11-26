# Etapa195 — núcleo de firmware asíncrono y binding inmutable

Se portó el núcleo de boot_firmware para la variante USB sin escritura DCPU a una máquina sin excepciones ni asignaciones. Mantiene las comprobaciones cold-start del núcleo, RMW/anchuras/W1C, preinit BBMCU, reset, dos descargas y polls finales. Entrega una acción por vez y no reemite ante fallo, cantidad incorrecta o cancelación. Los tokens bloquean completados antiguos/duplicados. Se conserva la pausa real de10ms,100 polls por condición y un plazo global30s. Registra la tentativa de mutación/reset antes de ceder la acción al backend. No promete restaurar CPU/firmware después de un fallo.

El binding exige SHA2564340bc322912f30f1793bb57a1747c23b15e40db2df628d6687d1ddf722f2828 del archivo retenido. Se usan prepare/packets anteriores para obtener las dos imágenes/212 paquetes originales. La prueba compara todos los bytes con boot_payload.hpp anterior y rechaza una modificación de archivo antes de producir salida. No hay sustitución de claves, firmas o seguridad.

Se comparó la traza completa del núcleo nuevo con la rutina anterior sobre el mismo backend simulado, tanto con vectores pequeños como con los212 paquetes vinculados. Coinciden lecturas/escrituras/pausas/envíos y contenido de todos los paquetes. Pasaron ASan/UBSan,101 inyecciones de fallo, ROM rechazada, timeout, cancelación, reloj/plazo, imágenes inválidas y short I/O en el último paquete. La simulación no demuestra latencias ni confirmaciones ROM de DriverKit en hardware.

IIG/build/enlace universal DriverKit27.2/target24.0 correctos. Las funciones start_bound_firmware, next_firmware_action y complete_firmware_action están enlazadas en el ejecutable. La DEXT incorpora las imágenes y solo ofrece GetBoundFirmwarePlan como consulta LOCALONLY. Start no comienza el motor. Stop cancela el estado del núcleo antes de cancelar el transporte, pero no hubo una ejecución de boot dentro de la DEXT.

Pendiente antes de habilitar arranque: ventanas WLAN/RF/identidad analógica y restauración, propiedad/fence del arranque, tablas posteriores, dispatcher/timers y drenaje. No existe API nativa de comienzo de reset hasta completar esa capa. La DEXT sigue sin instalarse; cero solicitudes USB y cero tráfico Wi-Fi nuevos. Tampoco se crea una interfaz de red ni se acredita identidad/liveness del firmware residente.

Evidencia y alcance: [driverkit-stage195-observation.json](driverkit-stage195-observation.json). Binario conservado: `artifacts/driverkit-stage195-ArcherUSB`. Firmware plan: `artifacts/driverkit-stage195-firmware-plan.json`.
