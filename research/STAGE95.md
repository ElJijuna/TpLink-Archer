# Etapa95 — prueba simultánea preparada, aún no ejecutada

Fecha: 2026-11-02.

El verificador físico confirmó canal9/MCC0 y TX pausado sin H2C ni reset.
Evidencia local: artifacts/external-probe-stage95-preflight.jsonl y stderr.

Se añadió un perfil tag6/secuencia6 con marcador exclusivo. El paquete conserva
los104 bytes y configuración del tag5 salvo identidad de secuencia/tag. No
reutiliza ningún marcador ni autoriza repetir el tag6 consumido.

El coordinador tools/external_probe_capture.py se ejecuta como usuario normal;
solo el capturador del Mac recibe sudo. Revalida el estado pausado del Archer,
inicia una captura independiente de hasta45s y espera su handoff. Antes de
crear el marcador y durante la admisión verifica que el capturador siga vivo,
que el PCAP tenga radiotap y que los paquetes completos disponibles indiquen
2452MHz e incluyan un beacon de Especial sin marca bad-fcs. El historial
queda ligado al ledger tras la limpieza92 y al PCAP pasivo94 por SHA256.

Los controles no prueban permanencia de canal durante toda la captura futura;
el PCAP final requiere auditoría. El coordinador espera a la captura incluso
si el firmware devuelve fallo terminal. Ese fallo conserva el marcador y no
se convierte en éxito por disponer de una captura externa. El helper intenta
restaurar el canal anterior y conserva cualquier error de restauración exacta.
Puede ser necesario reconectar Wi-Fi manualmente; no maneja credenciales.

Validación: compilación universal del perfil nuevo y capturador; suites C++ y
94 pruebas Python aprobadas. Tests cubren descriptor tag6, historial DIRTY o
incorrecto, rechazo de canal/radiotap equivocado, captura ausente antes de
marcar, conservación de marcadores anteriores y bloqueo de segundo intento.
No se ha ejecutado captura simultánea ni enviado el tag6. Avance52 %.

Ejecución desde Terminal, con contraseña únicamente en sudo:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v
python3 tools/external_probe_capture.py artifacts/external-probe-stage95
```

No ejecutar el coordinador entero con sudo. No reutilizar el directorio si
existe: conservar el resultado y revisar qué ocurrió antes de otro intento.
