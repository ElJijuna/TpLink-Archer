# Etapa122 — contraste DHCP broadcast preparado

2026-11-06. La captura116 demostró DISCOVER propio aceptado y reflejado por el AP mediante CCMP de grupo, sin OFFER. Su campo BOOTP flags era0: solicitaba respuesta unicast. RFC2131 sección4.1 describe la selección de entrega según ese bit y recomienda broadcast cuando el cliente no puede recibir unicast antes de configurarse. Aquí la recepción unicast física todavía no está verificada: solicitar broadcast permite contrastar esa dependencia mediante el camino de grupo ya observado. No demuestra que sea la causa del fallo anterior. El servidor también puede no responder por política, y la espera limitada sigue siendo otra incertidumbre.

Fuente: [RFC2131, sección4.1](https://www.rfc-editor.org/rfc/rfc2131.html#section-4.1).

## Cambio y validación

El próximo intento recuperado usa flags0x8000. La prueba compara ambos DISCOVER: solo cambia ese bit; longitud, IP/UDP, XID y opciones conservan el mismo formato. El perfil histórico y la auditoría de116 mantienen flags0. El auditor acepta ambos formatos y rechaza otros flags; pruebas con credenciales públicas ficticias verifican ambos caminos completos, OFFER pairwise y reflexión de grupo. La reflexión sigue sin completar la espera DHCP ni liberar contexto.

Se conserva instrumentación119, espera de tres segundos y política de un intento sin reintento automático. Suites C++ y123 pruebas Python aprobadas; binarios universales ARM/Intel compilados. La admisión real pasó sin prompt, acceso USB, consumo de secuencias o TX. La hipótesis broadcast aún no se ha comprobado por radio.

## Intento nuevo

Ligado a la recuperación121 y su UUID; ledger41 consumidas/215 libres, contexto/fence CLEAN. Secuencias Wi-Fi34–39, H2C41–49 y reporte de limpieza3 en la época nueva. Marcador nuevo `37ad-0104-special-channel9-wpa2-recovered-stage122-attempted`. El marcador116 permanece consumido y se comprueba su conservación. No ejecutar comandos116 antiguos: el coordinador vigente apunta exclusivamente a122.

Ejecutar una sola vez en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --wpa2-recovered artifacts/wpa2-stage122
```

Contraseña de Especial solo en el prompt local oculto. El ensayo realiza autenticación/asociación WPA2, M2/M4 y un DISCOVER cifrado con captura simultánea; espera OFFER coincidente y trata de desautenticar si la entrega está confirmada. No pide lease ni crea interfaz del sistema. Puede desconectar en0; no reintenta automáticamente. Compartir la salida completa sin contraseñas. Un fallo conserva reservas y necesita análisis, no repetición.

[Preparación, época y hashes](wpa2-stage122-observation.json). El avance ponderado se conserva en60 % hasta obtener evidencia funcional nueva.
