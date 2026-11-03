# Etapa99 — sondeo PHY nuevo listo para ejecución administrativa

Fecha: 2026-11-03. El usuario solicitó ejecutar medición, diagnóstico RF y
progresar hacia asociación/seguridad/datos.

Se preparó tag7/secuencia7 con marcador propio. El paquete mantiene los104
bytes, tasa CCK1, destino Especial y perfil no-RTS anterior salvo secuencia/tag.
La admisión exige el ledger59 reservadas/197 disponibles, limpieza terminal
del tag6 y evidencia física/CRC de etapa98. Se verificaron offline esos hashes.
Los fuentes pueden cambiar para incorporar el tag nuevo: la admisión liga la
evidencia cruda conservada, no afirma que el ejecutable viejo sea el actual.

El coordinador --phy conserva la captura externa hasta45s y solo admite el
sondeo después de observar Especial a2452MHz. El wrapper comprueba que haya
una única medición PHY antes/después si hubo escritura USB, cuatro contadores
de16bits por muestra, controles habilitados y deltas módulo coherentes.
Un fallo conserva marcador y evidencia; no reintenta.

El preflight físico volvió a confirmar canal9/TX pausado sin H2C ni reset.
Compilación Intel/ARM y suites C++/105 tests Python aprobados. No hay nuevo TX,
ni asociación, ni nueva calibración. Contexto CLEAN y197 secuencias; marcador
tag7 todavía ausente. sudo -n true indicó que falta contraseña: la ejecución
con captura independiente requiere autenticación interactiva del usuario.

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v
python3 tools/external_probe_capture.py --phy artifacts/phy-probe-stage99
```

La secuencia solicitada sigue condicionada por evidencia: primero ejecutar y
analizar PHY/captura; después localizar y corregir la diferencia de señal/RF;
solo tras TX/RX externo comprobado avanzar a asociación/seguridad y datos.
No se presentan los pasos2/3 como resueltos por preparar el sondeo. No se
necesita contraseña Wi-Fi para esta prueba de gestión. Avance52 %.

Evidencia de preparación: [phy-probe-stage99-preparation.json](phy-probe-stage99-preparation.json).
