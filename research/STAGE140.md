# Etapa140 — recuperación actual y continuación DHCP validada offline

2026-11-11. El Archer estaba de nuevo en modo instalador; tras la expulsión local informada por el usuario, una única orden SCSI terminó sin CSW válido. La enumeración posterior confirmó37ad:0104 y los registros confirmaron perfil frío, BB reset0. El CSW no se trata como confirmación.

La transición archivó el estado CLEAN anterior y verificó212 escrituras de firmware, estado listo, cero H2C y drenaje antes de publicar época a18950a3-9b75-4a74-b999-75be5760d9e3. Bootstrap/radio y canal9/calibraciones restaurados. Contexto pasivo creado y retirado con ocho ACK:386 transferencias,311 paquetes normales y208 beacons. Postflight210 registros correcto, contexto/fence CLEAN y215 secuencias libres. No hubo nuevas tramas Wi-Fi.

Se añadió selección DHCP experimental: REQUEST con la dirección ofrecida y servidor elegido; ACK solo si coincide transacción, chaddr, dirección y servidor, con lease no nulo. RELEASE usa ciaddr/IP de origen concedida y el servidor seleccionado, manteniendo identificador de cliente. Las opciones críticas duplicadas, tramas truncadas y respuestas fuera de orden se rechazan. El parser de OFFER histórico conserva su comportamiento para mensajes válidos.

Referencias: [RFC2131, selección y REQUEST](https://www.rfc-editor.org/rfc/rfc2131.html#section-3.1), [RFC2131, RELEASE](https://www.rfc-editor.org/rfc/rfc2131.html#section-4.4.6), [RFC2132, opciones DHCP](https://www.rfc-editor.org/rfc/rfc2132.html#section-9).

El modo nuevo usa ocho fases y reportes distintos para RELEASE y desautenticación. El consumidor DHCP se ejecuta después de autenticar CCMP; las pruebas comprueban que corrupción con FCS recalculada y replay no lo alcanzan. El validador exige ocho envíos únicos, ACK antes de RELEASE y los dos reportes TX_DONE en sus ventanas. Enviar RELEASE no confirma que el servidor lo haya procesado: esa limitación se registra explícitamente.

Suites C++ y125 pruebas Python aprobadas; modos histórico y nuevo compilados para ARM/Intel. No se ha comprobado ACK/RELEASE por radio ni configurado una dirección en macOS. La tarea de controlador utilizable sigue abierta, avance físico conservado en64 %.

[Recuperación y hashes](recovery-stage140-observation.json).
