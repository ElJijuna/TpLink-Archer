# Etapa82 — perfil de potencia de canal9 separado

2026-10-31. Avance52 %. Preparación offline; sin nuevo TX ni calibración física.

`tools/channel_bundle.py` genera ahora `trusted_channel9_world_power`, además
 del perfil existente de canal1. Ambos contienen197 escrituras para20MHz,
2G, dos transmisores y el perfil diagnóstico WW/noSAR previamente definido.
El tipo de escritura conserva el nombre histórico `Channel1BbWrite`; esto
no habilita por sí mismo su uso en las herramientas nativas de canal1.

La fuente fijada de Linux c03c1a14507dfaf8b856c2608f9ac216021fc222 establece:

- `phy.c:2535`, `rtw89_channel_to_idx`: canales1–14 usan `channel - 1`.
- `phy_be.c:1376`, `phy_fill_limit_20m_be`: CCK20/40, OFDM y MCS20 consultan
  el canal actual; para9 corresponde índice8 en elemento10 del firmware.
- `phy_be.c:1638–1658`, RU20: cinco tipos RU consultan el mismo canal;
  corresponde índice8 en elemento13.

Se mantienen independientes del canal las tablas por tasa, referencias y
SAR de este perfil. Las celdas ausentes mantienen el cero de inicialización
 del modelo original. No se afirma dominio regulatorio de Perú ni calidad RF.

Validación:85 tests Python pasan. El nuevo test contrasta las celdas de los
dos transmisores con las entradas crudas seleccionadas del firmware para9.
El prefijo completo que genera canal1 conserva SHA256
`579c24d2a27b3711c2843212abd71bfb43f260eb8ceff19267c5e6dbb108e6d0`.
La generación sigue vinculada por hash al firmware y efuse capturados.

Pendiente: integrar potencia9 en una transición con TX pausado, adaptar los
comandos de calibración al canal9, verificar retorno/limpieza y preparar
reserva de reporte e intento para un probe dirigido a Especial. No se ha
modificado el marcador de TX ni consumido secuencias del adaptador.
