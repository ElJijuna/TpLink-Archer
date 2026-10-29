# Etapa69 — ruta estática del LED

Fecha: 2026-10-29. Avance global: **50 %**, sin nuevo hito físico.

## Evidencia y alcance

Binario: `artifacts/windows/app/Driver/Win11/rtwlanu7.sys`, SHA256
`68f735cc46b25ce983997d7f8693b24ea75af3abaf42a2f4d69fb63ad5590332`.
El INF UTF-16 vincula 37ad:0104 con RTL8922au.ndi y LedPatternC9.reg:
LEDCtrlMode `0,5;`. El 5 es un modo, no una identificación de GPIO.

REA abrió el objetivo; la consulta de cadenas quedó bloqueada por timeout
de la lease existente de Hopper. El proveedor objdump de REA tampoco estuvo
disponible. Los resultados siguientes proceden del análisis PE y del LLVM
objdump ya instalado con Xcode; no son una ejecución observada de Windows.

## Observaciones del binario

- `rtw_hal_led_set_ctrl_mode`, VA 0x14021b73c, traduce modo5 a modo3.
- `mac_led_ctrl`, VA 0x1403e5b50, acepta LED0 y pasa pin8 al método de salida.
- `mac_set_led_mode_be`, VA 0x1403ffc20, traduce LED0 a GPIO8; el modo3
  llama al helper 0x1403e61a0 con modo1 y pin8.
- El helper de salida 0x1403ffec0 utiliza para pin8 el bit0 de 0x61.
  También llama al método indirecto de dirección en private_ops+0x628.
- La tabla candidata de operaciones contiene en +0x968 la función
  0x1403efed0. Esta recorre registros de 12 bytes en una tabla por pin.
  Para pin8, la tabla VA 0x14055ded0 incluye función0x1b con registro0x2d4,
  máscara0x0f y valor0x0f. La actualización preserva los demás bits.
  La función0x0e tiene otras dos entradas: 0x2d4/0x0f/0x06 y
  0x4e/0x20/0x20; no deben confundirse con modo GPIO software.

Artefactos locales: `artifacts/led-static-stage69.py`,
`artifacts/led-static-stage69.json`, `artifacts/led-pinmux-stage69.asm`,
`artifacts/led-gpio8-pinmux-stage69.json` y los cortes de ensamblado LED.

## Corroboración y límites

La implementación común LED de Linux rtw89, commit
`c03c1a14507dfaf8b856c2608f9ac216021fc222`, también usa GPIO8, pinmux0xf
y salida activa baja. Fuentes guardadas en `artifacts/rtw89-led.c` y
`artifacts/rtw89-led.h`. No basta para afirmar que el despacho específico
del RTL8922A de este binario usa todos esos controles de dirección.

Pendientes: confirmar el despacho de placa/chip y resolver private_ops+0x628,
crear una prueba corta con lectura y restauración de los bits modificados,
y confirmar visualmente el LED. No se han escrito registros GPIO en esta etapa.

El dispositivo se observó en modo instalador 0bda:1a2b. El helper identificó
el CD virtual como disk16; está pendiente la expulsión autenticada por el
usuario y verificar después 37ad:0104. No reutilizar disk16 sin verificarlo.
La prueba LED no necesita asociación ni transmisión Wi-Fi, pero su éxito
no probaría que el controlador de red está completo.

## Continuación: dirección y recuperación USB

Se localizó `_mac_set_sw_gpio_mode_be` en 0x1403ff260..0x1403ff42e.
Para pin8 selecciona registro16 de bits en 0x62 y máscara0x0101;
el argumento booleano establece ambos bits o limpia ambos, preservando
el resto. Su puntero está en 0x140655258, exactamente +0x628 de
0x140654c30, la tabla retornada por 0x1404291ec para argumento0.
Esto resuelve el método de dirección de esa tabla BE. Aún debe vincularse
la construcción de adaptador seleccionada con el chip concreto antes de
dar por completo el despacho. Corte: `artifacts/led-direction-stage69.asm`.

El usuario confirmó DKIOCEJECT=0. La enumeración posterior continuó en
0bda:1a2b. Un intento normal recibió sense18 y CSW con etiqueta0x39;
un único intento de recuperación BOT tuvo reset y clear-halt correctos,
pero devolvió etiqueta0x3a. Ambas difieren de la etiqueta enviada0x21436587.
Se rechazaron; no se atribuye éxito al comando ni se ignoró la correlación.
Logs: `artifacts/led-mode-switch-stage69b.*` y `led-mode-switch-stage69c.*`.
El helper óptico posterior ya no encontró exactamente un CD instalador,
aunque IOUSB conservaba DISK. Se solicitó reconexión física; no se han
escrito registros GPIO y el encendido visual sigue pendiente en ese punto.

## Pulso físico de GPIO8

Tras reconectar y obtener una nueva expulsión autenticada, un intento normal
de cambio devolvió sense18 y error de CSW. Posteriormente la enumeración
sí confirmó **37ad:0104 / 802.11be WLAN Adapter**. No se atribuye la transición
solo al comando que falló. Evidencia: `artifacts/led-mode-switch-stage69d.*`.

Se implementaron `include/archer/led_window.hpp` y `src/macos_led.cpp`.
La herramienta exige el USB exacto y el perfil CV1/AID6933 dentro de la
ventana de acceso existente; mantiene el bloqueo SequenceLedger y no envía
H2C, firmware ni paquetes Wi-Fi. La inspección usa el comando XTAL-SI de
lectura de identidad existente, por lo que no es una operación sin escrituras.

`--gpio8-pulse-3s` libera la dirección, selecciona GPIO, baja la salida y
habilita dirección durante tres segundos. Conserva los bits ajenos a las
máscaras. Al terminar o ante una excepción intenta restaurar los tres campos
y verifica cada readback. No garantiza restauración ante desconexión, cierre
forzado del proceso o fallo persistente del transporte.

La ejecución ARM salió0, stderr vacío. Valores observados:

| Registro | Antes | Durante | Restaurado |
|---|---:|---:|---:|
| 0x2d4, 8 bits |0xf6|0xff|0xf6|
| 0x61, 8 bits |0|0|0|
| 0x62, 16 bits |0|0x0101|0|

La ventana de acceso WLAN también completó su restauración. Logs:
`artifacts/led-inspect-stage69.*`, `artifacts/led-pulse-stage69.*`.
Se solicitó confirmación visual al usuario; el readback por sí solo **no
demuestra que el diodo se haya encendido**, ni que esta placa conecte GPIO8
al LED. El despacho estático de construcción específico sigue con el límite
descrito arriba; el ensayo valida los controles de registro sobre el dispositivo.

Compilación universal ARM/Intel correcta. `make test` pasa, incluidos los
64 tests Python y el nuevo test C++ de restauración: inyección después de
cada escritura inicial, excepción del observador y conservación de cambios
en bits ajenos. Log `artifacts/led-tests-stage69.log`. Intel sin prueba física.

```
make led-universal
./build/archer-led-universal --inspect
./build/archer-led-universal --gpio8-pulse-3s
```

Ejecutar desde la raíz del proyecto con el adaptador en modo Wi-Fi. La opción
de pulso es un experimento explícito; no instala un controlador de red.

## Confirmación visual — LED validado en ARM

El usuario no alcanzó a observar el primer pulso. Se añadió la opción fija
`--gpio8-pulse-15s`, se recompiló universal y pasó el test de restauración.
La repetición salió0, stderr vacío y restauración verificada, incluidos los
controles de acceso WLAN. Log: `artifacts/led-pulse-stage69-15s.*`.
El usuario respondió **«Sí, se encendió»** a la confirmación visual.

Queda validado el encendido físico mediante GPIO8 en este adaptador sobre
Mac ARM. Los registros se restauraron al estado anterior; no se dejó el LED
encendido permanentemente. Esto no prueba transmisión, asociación, calidad
RF ni integración de red macOS. Avance global50 %, Intel aún sin prueba física.

Para repetir con tiempo de observación:

```
./build/archer-led-universal --gpio8-pulse-15s
```

Los párrafos anteriores que indican confirmación pendiente describen el
estado previo a esta repetición; este resultado cierra esa comprobación.
