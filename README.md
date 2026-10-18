# Archer USB — investigación y base de controlador

Proyecto experimental para TP-Link USB `37ad:0104`, con destino macOS Intel/Apple Silicon y Linux posteriormente.

**Estado: diagnóstico y protocolo USB parcial. Todavía no es un controlador Wi-Fi funcional.** No instala extensiones ni modifica el dispositivo.

## Ejecutar

Con Xcode Command Line Tools y Python 3:

```sh
make test
make probe
make universal
python3 tools/inspect_inf.py /ruta/al/paquete/extraido
```

`probe` consulta IORegistry, muestra únicamente el adaptador objetivo y devuelve 1 si está ausente o 2 si falla la enumeración. El ejecutable universal contiene arm64 y x86_64; eso no sustituye pruebas en un Mac Intel físico.

`register_request()` construye los ocho bytes del encabezado de control para registros, sin enviar solicitudes. Su dirección de 24 bits y anchuras 1/2/4 proceden de la implementación pública rtw89. No contiene secuencias de encendido, calibración ni firmware.

## Evidencia y propuesta

Consultar [research/FINDINGS.md](research/FINDINGS.md) y [research/PROPOSAL.md](research/PROPOSAL.md). Los paquetes originales se mantienen en `artifacts/`, excluidos de Git. No se incluye una licencia de redistribución para binarios TP-Link.

El INF oficial vincula este USB ID a `RTL8922au.ndi`. Esto identifica la familia que Windows selecciona; el modelo comercial exacto sigue sin comprobarse físicamente.
