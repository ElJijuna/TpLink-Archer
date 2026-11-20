# Etapa165 — contraste de reporte TX antes del lease

2026-11-24. El replay del analizador encuentra el reporte C2H1/9/1 de desautenticación en138 (tag6, TX_DONE/count1), pero ninguno en162:446 paquetes normales,464 metadatos PPDU y ocho C2H de control. La auditoría163 autenticó OFFER/ACK y reprodujo REQUEST/RELEASE; eso no confirma entrega de RELEASE. El contador PHY avanzó durante RELEASE sin demostrar emisión.

Hipótesis pendientes: reporte no generado para este descriptor de datos, entrega por otra ruta RX o transmisión incompleta. El replay descarta un tag C2H alternativo dentro de las transferencias guardadas; no descarta pérdida anterior al USB. No hay causa raíz ni corrección RF comprobada.

El ensayo165 modifica una variable de DISCOVER: solicita un reporte con tag12. El descriptor mantiene los campos de datos, secuencia y CCMP; no cambia tasa, colas, criptografía ni límites RF. Antes de retransmitir DISCOVER o enviar REQUEST exige TX_DONE/count1. Un OFFER por sí solo no supera esta barrera. Si falta el reporte, se detiene sin solicitar lease; mantiene reserva/contexto incierto y no relanza el ensayo. Si aparece el reporte, conserva el protocolo existente REQUEST/ACK/RELEASE y desautenticación (tags13/14); un reporte RELEASE tampoco confirma procesamiento por el servidor.

Ligado a recuperación164, época12fc87ad-9c12-428d-9ca3-a060ded67535, contexto/fence CLEAN,215 secuencias libres y210 registros actuales verificados. Marcador nuevo165;162 permanece consumido. Mantiene hasta tres respuestas M1 con replay creciente y, solo tras el reporte válido, una retransmisión DISCOVER con PN/secuencia nuevos. Sin interfaz ni configuración IP de macOS.

La regresión nativa rechaza reporte ausente, fallido o count incorrecto. El validador del resultado exige reporte único antes de retry/REQUEST y rechaza evidencia tardía. Pasaron133 pruebas Python, suites C++ y compilación ARM/Intel. Admisión real comprobada sin credenciales, USB ni TX nuevos. El ensayo físico aún no se ejecutó.

Ejecutar una sola vez en Terminal como usuario normal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --dhcp-supplicant artifacts/supplicant-stage165
```

Contraseña de Especial solo en los dos prompts locales ocultos; compartir salida sin contraseña. Puede desconectar en0. Si supera la barrera TX solicitará lease temporal e intentará RELEASE; sin confirmación del servidor. No repetir162 ni borrar reservas.

[Preparación y hashes](supplicant-stage165-observation.json).
