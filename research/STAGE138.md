# Etapa138 — intento nuevo con confirmación local de contraseña

2026-11-08. Vinculado a recuperación137, UUIDda251133-4f2b-4ff7-a08a-8ec21a8ee778, contexto/fence CLEAN y215 secuencias libres. Sustituye el intento133 consumido, que se detuvo tras M2 y cuya contraseña fue informada incorrecta por el usuario.

Se mantiene un solo DISCOVER broadcast y escucha DHCP de hasta10 segundos, recortada por el límite global20s; gracia3s exclusiva de limpieza. La ampliación todavía no se ha probado por radio. No adquiere lease ni crea interfaz del sistema. OFFER autenticado y coincidente sigue siendo el criterio de completar DHCP; reflejar DISCOVER no libera contexto.

El flujo de radio pide ahora la contraseña dos veces mediante prompts locales ocultos. Si difieren, se detiene antes de crear el pipe de credenciales, comprobar registros, iniciar captura o consumir el marcador. Ambas entradas temporales se borran de los buffers mutables; no se guardan ni se imprimen. La igualdad no comprueba que la contraseña sea correcta para el AP. Puede quedar creado el directorio de salida aunque no haya transmisión; no debe confundirse su existencia con un marcador consumido. Las auditorías offline conservan su prompt único.

La prueba nueva confirma que una discrepancia no crea pipe y que la coincidencia transmite la credencial ficticia solo por el pipe privado. Suites C++ y124 pruebas Python aprobadas; binario ARM/Intel compilado. Admisión real verificada sin USB, prompt, reservas o TX nuevos.

Secuencias Wi-Fi52–57, H2C41–49, reporte de limpieza6, marcador nuevo `37ad-0104-special-channel9-wpa2-recovered-stage138-attempted`, todavía ausente. Los marcadores históricos116/122/133 se comprueban intactos.

Ejecutar una sola vez en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo -v && python3 tools/external_probe_capture.py --wpa2-recovered artifacts/wpa2-stage138
```

Introducir contraseña de Especial solo en los dos prompts locales ocultos y compartir salida completa sin contraseñas. Captura simultánea; puede desconectar en0. No reintenta automáticamente; si falla, conservar resultados y reservas para análisis. No repetir comandos133 anteriores.

[Preparación y hashes](wpa2-stage138-observation.json). Avance estimado conservado en60 %.
