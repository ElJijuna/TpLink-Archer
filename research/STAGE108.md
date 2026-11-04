# Etapa108 — instalador USB tras reconexión

2026-11-04. La reconexión del usuario cambió la enumeración a0bda:1a2b, modo CD instalador Realtek.37ad:0104 aún no está disponible; no se ha comprobado todavía el perfil frío de registros Wi-Fi.

El inspector nativo verificó exactamente un `RTK Driver Storage Media`, ancestro USB0bda:1a2b, unidad completa/removible/no escribible `/dev/disk18`, major1/minor48. La expulsión por diskutil no pudo operar sobre la unidad. El helper USB inspeccionó configuración de almacenamiento correctamente, pero tanto apertura como seize devolvieron -536870203 antes de enviar SCSI. No se envió START STOP UNIT y no se interpreta el intento como cambio de modo exitoso.

El helper de expulsión óptica valida identidad y números de dispositivo de nuevo en cada ejecución antes de DKIOCEJECT. Requiere root; sudo no interactivo devolvió `a password is required`. Por tanto queda pendiente una ejecución local en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo ./build/archer-optical-eject-universal --eject-installer
```

No usa un número de disco fijo: selecciona únicamente el instalador verificado en vivo y rechaza otro medio. La contraseña se introduce en Terminal.

Se cambió el bloqueo de los helpers de instalador de SequenceLedger a WorkflowLock: el modo de almacenamiento no ejecuta firmware/H2C, y la barrera DIRTY de la época debe permanecer intacta mientras se permite recuperar el modo USB. Se mantiene el bloqueo compartido frente al coordinador exclusivo y todas las verificaciones de identidad del instalador. No se reserva, reemplaza ni reinicia el ledger. Helpers universales ARM/Intel compilados; suites C++ y118 pruebas Python aprobadas.

Estado: contexto y transición DIRTY, ledger anterior idéntico con143 secuencias libres,0 H2C/frames/reset nuevos. Tras la expulsión se comprobará la enumeración y el perfil frío antes de continuar la transición107. No repetir WPA2.

Evidencia: [observación108](reconnect-stage108-observation.json).
