# Monitor PC-ESP32

Aplicativo GTK3 que monitora CPU, RAM e processos do PC e envia os dados via serial para um ESP32 com display.

Funciona na bandeja do sistema (systray) usando AppIndicator — compatível com Ubuntu, Debian, Fedora, Arch, openSUSE, Mint, etc.

## Estrutura

```
monitor/
├── src/
│   ├── sysinfo.h    — protótipos: CPU, RAM, processos
│   ├── sysinfo.c    — coleta dados via /proc e ps
│   ├── serial.h     — protótipos: porta serial
│   ├── serial.c     — abre/escreve/fecha /dev/ttyACM0 (115200 8N1)
│   ├── daemon.h     — estado compartilhado + API da thread
│   ├── daemon.c     — thread: detecta ESP32 → envia dados → reconecta
│   └── main.c       — app GTK3 + AppIndicator: ícone, tooltip, menu
├── Makefile
├── monitor.desktop  — entrada autostart (~/.config/autostart)
└── monitor           — binário compilado
esp/
├── tela_zica_codigo.ino  — firmware ESP32 (LVGL + ILI9341)
└── lv_conf.h              — configuração do LVGL
```

## Dependências

O Makefile detecta automaticamente qual biblioteca de AppIndicator está instalada (Ayatana ou libappindicator).

| Distruibuição | Comando de instalação |
|---|---|
| Ubuntu / Debian / Mint | `sudo apt install build-essential libgtk-3-dev libayatana-appindicator3-dev` |
| Fedora | `sudo dnf install gcc make gtk3-devel libappindicator-gtk3-devel` |
| Arch Linux | `sudo pacman -S gtk3 libappindicator-gtk3` |
| openSUSE | `sudo zypper install gcc make gtk3-devel libappindicator3-devel` |

## Compilar

```bash
cd monitor
make clean && make
```

Gera o binário `./monitor`.

## Instalar (inicia junto com o sistema)

```bash
sudo make install
```

Isso copia:
- Binário → `/usr/local/bin/monitor-pc-esp32`
- Desktop entry → `~/.config/autostart/monitor-pc-esp32.desktop`

Para desinstalar:

```bash
sudo make uninstall
```

## Usar

1. Conecte o ESP32 via USB — ele aparece como `/dev/ttyACM0`
2. Execute `./monitor` (ou reinicie o PC se instalou no autostart)
3. Um ícone aparece na bandeja do sistema:
   - **Verde**: conectado e enviando dados
   - **Vermelho**: desconectado, tentando reconectar a cada 2s
4. Clique no ícone para ver status, CPU, RAM e opção Sair

## Protocolo Serial

Formato enviado ao ESP32 (115200 baud, 8N1):

```
C:<cpu%>;R:<ram%>;P:<proc1>,<ramMB1>,<proc2>,<ramMB2>,...;
```

Exemplo:
```
C:23;R:45;P:firefox,512,chrome,384,node,256,;
```

## Permissão da Porta Serial

Se o app não conseguir abrir `/dev/ttyACM0`, adicione seu usuário ao grupo `dialout`:

```bash
sudo usermod -a -G dialout $USER
```

Depois faça logout/login ou reinicie.

## Porta Serial Diferente

Edite `DEFAULT_SERIAL_PORT` em `src/serial.h` e recompile.