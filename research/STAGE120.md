# Etapa120 — reconexión detectada en modo instalador

2026-11-06. Tras confirmar el usuario la desconexión de cinco segundos, la enumeración actual muestra Realtek DISK USB0bda:1a2b. Todavía no se ha confirmado la interfaz Wi-Fi37ad:0104 ni el perfil frío.

La expulsión específica del instalador mediante `sudo -n` se detuvo con salida1: `sudo: a password is required`. El helper no llegó a ejecutarse. No hubo nueva carga de firmware, H2C ni transmisión Wi-Fi; las reservas se conservan.

Para continuar, el usuario debe ejecutar en su Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo ./build/archer-optical-eject-universal --eject-installer
```

La contraseña de sudo se introduce solo en Terminal. El helper comprueba la identidad del CD Realtek antes de expulsarlo. Después deben verificarse enumeración Wi-Fi y perfil frío antes de continuar la recuperación. No repetir WPA2 ni borrar reservas.

[Evidencia y hashes](recovery-stage120-observation.json).
