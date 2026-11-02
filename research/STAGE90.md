# Etapa90 — auditoría ampliada DPK/TSSI, sin reinterpretar calidad RF

2026-11-01. Avance52 %. Análisis offline; sin nuevo acceso USB, H2C o trama.

## Diagnósticos reproducibles

Se añadió `archer-rfk-capture-audit`, compilado Intel/ARM, para recorrer las
capturas USB RFK en orden y emitir JSONL con campos detallados DPK/TSSI.
El decodificador DPK conserva ahora índice, TXAGC, térmico, gain scaling,
bytes DC-I/DC-Q y correlación, según la estructura primaria de34 bytes de
`rtw89-fw.h:5213`. No se asignan unidades/signos/umbrales a campos que la
fuente solo imprime. El byte reservado sigue reservado.

Comparación de capturas físicas conservadas:

| Campo | Canal1 histórico, etapa42 | Canal9, etapa84 |
|---|---|---|
| DPK versión |13|13|
| Índice |[0,0]|[0,0]|
| Path status |[4,4]|[4,4]|
| TXAGC crudo |[54,55]|[52,53]|
| Térmico crudo |[62,62]|[59,59]|
| Gain scaling crudo |[91,91]|[91,91]|
| Timeout/overflow |[0,0]/[0,0]|[0,0]/[0,0]|
| Códigos TSSI |16 ceros|16 ceros|
| Alignment state |[0,0,0,0]|[0,0,0,0]|

DPK contiene resultados de ganancia/corrección/correlación no nulos. Eso no
resuelve qué significa path4 ni acredita calidad RF. El patrón TSSI a cero
ya estaba presente en canal1: no es una anomalía demostrada introducida por
el retune9. En9 se conservaron dos informes TSSI, no solo el último.

## Contraste con el driver Windows

REA reconoció el PE x86_64 del driver con SHA256
`68f735cc46b25ce983997d7f8693b24ea75af3abaf42a2f4d69fb63ad5590332`.
La consulta de cadenas falló por lease de Hopper cuyo propietario no
respondió; no se obtuvo descompilación REA. Se conservó el error y se cerró
el target. No se modificó instalación/configuración ni sesión ajena.

Se continuó con LLVM objdump local, sin ejecutar el driver. El consumidor
Windows en0x1402f3830 copia56 bytes y convierte las matrices de alineación
con el mismo esquema alto/bajo que el decodificador. Copia los estados a
campos de host desde el offset48. En0x1402f57ca..0x1402f580b, la presentación
`Alimk Status A/B` selecciona `Disable` si el byte es0 y `Enable` si no lo es.
También se observan escrituras0/1 de esos campos en la rutina referenciada
por la cadena `_halrf_tssi_alimentk_phy0_8922a`.

**Alcance:** esta es una observación estática del consumidor/diagnóstico de
Windows, no una validación del algoritmo productor en el firmware0.35.41.
Una etiqueta Enable/Disable de alineación no es un resultado global TSSI ni
una medición de potencia. Cero puede representar un estado no habilitado/no
obtenido; no se convierte automáticamente en éxito o fallo. La ruta DPK del
driver inspeccionado tampoco demuestra la semántica del path4 del ABI Linux.
No se cambiaron esos predicados ni los indicadores quality_validated=false.

[windows-rfk-stage90-evidence.json](windows-rfk-stage90-evidence.json) contiene
rangos seleccionados, instrucciones, direcciones, bytes/hash y cadenas. No
presenta rangos del analista como límites de funciones de un descompilador.

## Uso y validación

```
make rfk-capture-audit-universal
./build/archer-rfk-capture-audit-universal artifacts/channel9-calibration-stage84-evidence/capture
```

`make test`:91 tests Python y suites C++. Tests de bytes distinguen todos
los campos DPK nuevos sin inferir unidades. El calibrador nativo y el auditor
compilan universal; Intel sigue sin prueba física.
[rfk-diagnostics-stage90-observation.json](rfk-diagnostics-stage90-observation.json)
conserva los replays comparados y hashes. Artefactos con prefijos
`artifacts/rfk-audit-stage90-` y `artifacts/rea-stage90-`.

No se modificó el estado del adaptador. El journal conserva CLEAN y197
secuencias; el último estado físico verificado sigue siendo el de89.

Pendiente: evidencia del productor DPK/TSSI para esta versión o contraste
RF externo. No se ha resuelto la ausencia de respuesta al sondeo unicast ni
habilitado asociación/interfaz de red.
