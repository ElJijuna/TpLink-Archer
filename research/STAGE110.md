# Etapa110 — nuevo intento compilado; modo instalador bloquea la admisión

2026-11-05. Se compiló un intento WPA2 distinto con secuencias802.11 22–27, marcador nuevo y reporte de limpieza1 en una época de firmware nueva. El reporte1 de una época anterior no autoriza su uso en la actual: la admisión requiere una frontera fría y contexto comprobados para la identidad de época vigente. Ninguna reserva anterior se elimina. El marcador nuevo aún no existe y el intento no se ejecutó.

La comprobación física actual no pudo abrir37ad:0104. La enumeración mostró0bda:1a2b y el inspector verificó nuevamente el CD instalador Realtek, `/dev/disk18`. Eso invalida tratar la recuperación109 como estado vivo. No se deduce qué evento externo causó este nuevo modo. El preflight se detuvo sin H2C ni tramas.

Se reforzó la identificación de épocas: el coordinador archiva `.epoch-id`, conserva la identidad anterior si falla y publica un UUID nuevo únicamente tras la frontera de firmware verificada y antes de liberar la barrera. La admisión WPA2 exige que esa identidad coincida con la evidencia, aunque los bytes y conteos del ledger coincidan. No se fabricó una identidad para la recuperación histórica109, anterior a este cambio. La siguiente recuperación debe establecerla mediante el protocolo completo.

Pruebas: regresión de directorio real, rechazo de efectos extra en preflight, continuaciones fallidas/satisfactorias, preservación de identidad anterior y rechazo de otra época con el mismo ledger. Suites C++ y120 pruebas Python pasan. El ejecutable WPA2 está compilado ARM/Intel; la admisión está explícitamente bloqueada (`blocked_installer_mode`), no lista para ejecución.

Los archivos de contexto/fence permanecen CLEAN y el ledger registra215 libres de la época109, pero eso **no certifica firmware vivo** estando en modo instalador. Los marcadores y la evidencia anteriores siguen intactos.

La expulsión nativa volvió a requerir autenticación local (`sudo: a password is required`). Ejecutar solo:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo ./build/archer-optical-eject-universal --eject-installer
```

El helper selecciona y verifica el CD Realtek en vivo; no depende de un número de disco fijo ni opera sobre discos de usuario. Compartir la salida para continuar con enumeración, estado frío y nueva frontera verificada. No ejecutar todavía `--wpa2-once` ni `--wpa2-recovered`.

Evidencia: [observación110](wpa2-stage110-observation.json).
