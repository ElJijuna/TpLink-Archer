# Etapa128 — sesión nueva en modo instalador

2026-11-06. Después de verificar ausencia USB en127, la reconexión produce Realtek DISK0bda:1a2b con sesión13504709733536 y objeto0x100258925, distintos de los anteriores. Está comprobada una nueva enumeración; el perfil frío todavía no se puede verificar en el modo instalador.

La expulsión óptica mediante sudo no interactivo se detuvo antes del helper: salida1, `sudo: a password is required`. Hace falta ejecutarla en Terminal local:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo ./build/archer-optical-eject-universal --eject-installer
```

Compartir salida completa; contraseña solo en Terminal. Después verificar interfaz Wi-Fi y registros fríos antes de recuperar. Sin firmware, H2C o TX nuevos. Reservas conservadas; no ejecutar WPA2 todavía.

[Evidencia y hashes](recovery-stage128-observation.json).
