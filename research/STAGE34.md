# MAC, radio y calibración inicial: estado reproducible

La prueba física final es `artifacts/archer-rfk-stage34 --initial-rfk artifacts/rfk-stage34`.
Terminó con código0: PRE_NOTIFY, DACK y RXDCK informaron OK; DACK reportó sus
siete flags de timeout y DACK_FAIL a cero; RXDCK reportó canal1, banda2GHz,
20MHz y timeout0 en ambas rutas. El manifiesto
`mac-radio-rfk-stage34-observation.json` conserva hashes de ejecutables, trazas
y fixtures. Los fixtures de calibración no contienen tráfico recibido del aire.

Antes se cargó firmware principal0.35.41.0/BBMCU0 y se aplicaron DLE SCC,
HFC y la parte DMAC descrita en `DMAC_INIT.md`. `radio-init-stage32e.jsonl`
conserva la aplicación de174 escrituras BB,1085 entradas radioA,1083 radioB y
1200 entradas NCTL. El éxito de envío de una tabla no prueba cada celda RF:
RF5 cambió de0 a1 en ambas rutas; RF0 cambia con el funcionamiento del firmware.

## Correcciones verificadas

- ADDA: se omitían los controles del inicio de `rtw8922a_pwr_on_func`:
  borrar0x244 bit28, habilitar LDO0/1 en0x16 bits3:2 y quitar aislamiento
  ADDA en0x84 bits21:20. La primera prueba tenía DRCK timeout1 y MSBK3.
  Tras habilitar ADDA se obtuvieron coeficientes y DRCK0, pero MSBK3 persistió.
  Solo después de recargar firmware y completar la nueva secuencia se obtuvo
  un informe limpio. No se demuestra cuál de esos pasos eliminó MSBK ni que
  el flag previo fuese persistente.
- CMAC compartido: habilitar0xe000 mask0x40000007 antes de DMAC.
- HFC: el hardware devuelve condición CH12 efectiva0 tras solicitar1.
  Linux lee el valor efectivo y no exige igualdad para esa condición.
  Se conserva y reporta la diferencia0x400; no se afirma que1 quedó aplicado.
- Corte MPDU:0x9c94 devuelve0x200e05f0 tras escribir0x210e05f0,
  incluso con CMAC compartido habilitado. El bit ausente es CUT_CTRL bit24,
  no ADDRCAM bit29. Linux no comprueba ese bit tras escribir. Se mantiene la
  solicitud, se valida el resto y se devuelve el valor efectivo. Su causa y
  repercusión funcional no están determinadas; no se afirma soporte de corte.
- RFK: el estado físico ocupa14 bytes, frente al prefijo de10 del source.
  Los cuatro bytes extra se conservan opacos. Un estado OK no basta:
  los logs detallados pueden indicar timeout o llegar después del estado.
  El ejecutor espera ambos, conserva las capturas y no continúa tras fallo.

## Contexto inicial y límites

`rtw89-core.c:396` define el canal inicial desde la primera entrada2GHz,
canal1 sin HT/20MHz. `rtw89_core_start:6717` ejecuta RFK late-init después
de inicializar BB/RF; no requiere ejecutar antes `__rtw89_set_channel`.
`rtw8922a_rfk_init` pone a cero el estado MCC y late-init envía PRE_NOTIFY,
DACK y RXDCK. El ejecutor porta ese contexto inicial; no sustituye el
coordinador de cambio de canal ni autoriza transmisión de red.

Faltan el resto de `cmac_init_be`/configuración MAC, la integración del cambio
de canal con tablas de ganancia/efuse/trim/potencia, y TXGAPK/IQK/DPK/TSSI.
Los planes offline están en `channel_init.hpp`, `channel_transition.hpp` y
`tssi_protocol.hpp`; sus flags no constituyen evidencia de ejecución.
`initialize_channel1` sigue rechazando la ejecución completa. TSSI continúa
bloqueado hasta validar sus entradas y el contexto de potencia.

La vía experimental USB sigue omitiendo explícitamente el acceso DCPU0x888
inaccesible observado anteriormente; no es una solución general para todas
las variantes. La compilación universal no prueba el hardware Intel. Linux
y la integración de interfaz Wi-Fi aún no están implementados.

## Comprobación

`make test` incluye28 suites C++ y46 pruebas Python. Los targets universal
compilan las herramientas para arm64/x86_64. Los binarios de las pruebas
físicas se guardan antes de ejecutarlos y sus hashes no cambian al recompilar.

Las opciones de inicialización son experimentales y dependen del estado
registrado. `--resume-loaded-scc-radio` solo acepta firmware ya cargado,
ADDA sin aislamiento y BB aún sin liberar; no equivale a recuperación general.
No reiniciar el ledger de secuencias al recargar firmware.
