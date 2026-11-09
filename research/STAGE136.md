# Etapa136 — nueva sesión instalador tras ausencia verificada

2026-11-08. Tras ausencia comprobada135, el Archer reconectado enumera Realtek0bda:1a2b, sesión16569947150576. El perfil frío aún no está verificado. La expulsión óptica no interactiva no llegó al helper: sudo exige contraseña local.

Ejecutar en Terminal:

```sh
cd /Users/pilmee/Documents/Github/Tp-link-Archer
sudo ./build/archer-optical-eject-universal --eject-installer
```

Compartir salida completa; contraseña únicamente en Terminal. Después verificar interfaz Wi-Fi y perfil frío. No repetir WPA2133, borrar reservas ni dar por recuperada la época anterior. Sin recarga, H2C o TX nuevos.

[Evidencia y hashes](recovery-stage136-observation.json).
