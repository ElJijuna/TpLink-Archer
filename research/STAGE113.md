# Etapa113 — M2 entregado a USB, sin M3; corrección de descriptor pendiente por radio

2026-11-05. El intento112 recibió todos los ACK de creación del contexto. Especial aceptó autenticación y asociación, estado0/AID1. El receptor nativo obtuvo cuatro M1 EAPOL versión2/key_info0x008a. M2 se entregó por USB09,217 bytes/status0. No llegó M3 durante la espera. No se transmitieron M4, DHCP ni desautenticación; la fase pendiente impide considerarlos seguros para limpieza automática de Wi-Fi.

La retirada H2C del contexto recibió sus tres confirmaciones exigidas y los callbacks se drenaron. Aun así la reserva permanece DIRTY porque el envío BE no quedó confirmado por radio. La barrera de época sigue CLEAN, y la identidad no cambió. Secuencias41–49 consumidas,206 libres. No se reutilizan el marcador112 ni las secuencias22–27 de Wi-Fi.

## Correlación de radio y estructura

La captura externa contiene2818 registros,1460 CRC válidos,1357 inválidos,1 truncado y381 beacons válidos de Especial. Se corroboran autenticación (279/281) y asociación (283/285). Las seis tramas válidas relacionadas con nuestra MAC son gestión/ACK. Un examen adicional de todos los registros, incluidos CRC inválidos, solo encontró dos transmisiones cuyo TA coincide exactamente con nuestra MAC (ambas solicitudes de gestión); no encontró M2 coincidente con la trama preservada, ni normalizando la duración. Esto no demuestra ausencia absoluta de emisión o energía RF.

El auditor nativo independiente encontró cuatro EAPOL dirigidos a nuestra MAC: todos versión2, key_info138, longitud95 (M1), y ningún otro EAPOL candidato. M2 preservado tiene153 bytes802.11, versión2, key_info0x010a, key_length0, replay correspondiente al primer M1, SNonce distinto, MIC no nulo y RSN igual al de asociación. Su MIC no se pudo verificar offline porque contraseña/PMK no se conservaron. **No se determina si la contraseña era correcta.**

## Diferencia de descriptor corregida localmente

Se encontró un error del descriptor construido por el proyecto: heredaba `SW_MLD=1` para datos. La fuente conservada `rtw89-core.c:914–917` lo activa para gestión, pero `rtw89_core_tx_update_data_info` lo pone false (línea1141). El constructor de DWORD3 lo vincula con `BE_TXD_BODY3_IS_MLD_SW_EN` (1733–1737); la definición está en `rtw89-txrx.h` bit13.

El M2 enviado llevaba DWORD3=0x2018. Para esa secuencia, el perfil de datos de la fuente exige0x0018. Una regresión sobre el constructor real falló antes del arreglo y pasa después. Se mantiene SW_MLD para gestión y se desactiva para EAPOL/datos. Suites C++ y120 pruebas Python pasan; compilación ARM/Intel correcta. **La corrección aún no está validada por radio y no prueba por sí sola la causa de la ausencia de M3.** El contexto todavía utiliza CAM NO_LINK; una ruta de datos completa puede requerir trabajo adicional.

Se añadieron snapshots PHY antes/después de cada fase y al salir por espera fallida, antes de la retirada del contexto. Son lecturas, con deltas módulo16 y sin atribuir por sí solos emisión. El postflight del intento ya cerrado ve contadores0 y no permite reconstruir un delta M2; no se interpreta como ausencia de TX. La nueva instrumentación solo está compilada, no ejecutada por radio.

## Estado y siguiente paso

Postflight físico:210 registros coinciden, concesión original0x01775774 restaurada y TX pausado. No se marca CLEAN la reserva. Antes de otra transmisión se necesita una frontera fría verificada que archive esta época y descarte la entrega incierta. No repetir el comando112 ni borrar marcadores.

Para preparar esa frontera, desconectar físicamente el Archer, esperar5 segundos, reconectar y expulsar únicamente su CD instalador mediante el helper verificado en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo ./build/archer-optical-eject-universal --eject-installer
```

Compartir la salida. El siguiente intento deberá tener marcador/secuencias nuevos y admisión vinculada a la nueva identidad de época. No hay seguridad WPA2, lease ni interfaz de red establecida.

Evidencia: [observación113 y hashes](wpa2-stage113-observation.json).
