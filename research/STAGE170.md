# Etapa170 — contraste ACQ ligado a recuperación169

2026-11-24. Preparación nueva tras la recuperación167 fallida y la recuperación169 completa. El intento168 no se ejecutó. Marcador nuevo170, época88c619a3-cd2a-4836-8893-f961aefc92bb, contexto/fence CLEAN y215 secuencias libres.

Mantiene el paquete y la barrera de165. La única variación funcional del contraste es habilitar CCTLINFO_G7_W0_ACQ_RPT_EN(bit20) en el contexto supplicant: W0 pasa de0x00200004 a0x00300004, con el bit incluido en la máscara. El origen165 confirmó ausencia de ese bit en el paquete de control y OFFER válido sin reporte TX. La opción ACQ no se activa en los demás contextos. No se modifica tasa, colas, claves ni límites RF. El efecto sobre reportes todavía no está comprobado por radio.

DISCOVER solicita tag12 y debe recibir TX_DONE/count1 antes de retransmitir DISCOVER o enviar REQUEST; si falta, se detiene sin solicitar lease. Si supera esa barrera, continúa REQUEST/ACK/RELEASE y desautenticación con tags13/14. Hasta tres respuestas M1 con replay creciente y una retransmisión DISCOVER con PN/secuencia nuevos; no se relanza el ensayo completo. Puede conceder lease temporal e intenta RELEASE, sin confirmación del servidor. No configura IP/rutas ni crea interfaz macOS.

Regresión a través del constructor real y ACK hasta la tabla CMAC: solo cambia bit20, la máscara lo incluye, ACQ sin MGQ se rechaza. Suites C++ y133 pruebas Python aprobadas; binario ARM/Intel compilado. Admisión170 real aprobada sin credenciales, USB ni TX nuevos. No se reutilizan165/162.

Ejecutar una sola vez como usuario normal en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --dhcp-supplicant artifacts/supplicant-stage170
```

Contraseña de Especial solo en los dos prompts locales ocultos; compartir salida sin contraseña. Captura simultánea; en0 puede desconectarse. Conservar reservas y resultados si se detiene.

[Preparación y hashes](supplicant-stage170-observation.json).
