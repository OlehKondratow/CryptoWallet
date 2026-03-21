# Host Runner Installation & Management Guide

## 🚀 Quick Start

### Method 1: Automated Setup (Recommended)

```bash
cd /data/projects/CryptoWallet/infra

# Make script executable
chmod +x setup-host-runner.sh

# Run setup (will prompt for token)
./setup-host-runner.sh
```

The script will:
1. Download Act Runner v0.3.0
2. Generate and configure `config.yaml`
3. Register with Gitea
4. Create systemd user service
5. Start the runner

### Method 2: Manual Installation

#### Step 1: Download & Extract

```bash
cd ~
mkdir -p gitea-runner
cd gitea-runner

wget https://dl.gitea.io/act_runner/v0.3.0/act_runner-v0.3.0-linux_amd64.tar.gz
tar -xzf act_runner-v0.3.0-linux_amd64.tar.gz
rm act_runner-v0.3.0-linux_amd64.tar.gz
chmod +x act_runner
```

#### Step 2: Generate Config

```bash
./act_runner generate-config > config.yaml
```

Edit `config.yaml` - set `runner.labels`:
```yaml
runner:
  labels:
    - ubuntu-latest
    - host
```

#### Step 3: Get Registration Token

1. Go to: http://localhost:3000/admin/runners
2. Click "Create new Runner"
3. Copy the token

#### Step 4: Register

```bash
./act_runner register \
  --instance http://localhost:3000 \
  --token YOUR_TOKEN_HERE \
  --name host-runner-$(hostname) \
  --no-interactive
```

#### Step 5: Test Run

```bash
./act_runner daemon
```

Should see:
```
time=... level=info msg="Runner registered successfully."
```

Press `Ctrl+C` to stop.

#### Step 6: Systemd Service

Create `~/.config/systemd/user/gitea-runner.service`:

```ini
[Unit]
Description=Gitea Act Runner (Host)
After=network.target

[Service]
Type=simple
WorkingDirectory=%h/gitea-runner
ExecStart=%h/gitea-runner/act_runner daemon
Restart=on-failure
RestartSec=10

[Install]
WantedBy=default.target
```

Enable & start:
```bash
systemctl --user daemon-reload
systemctl --user enable gitea-runner
systemctl --user start gitea-runner
```

---

## 📊 Verify Setup

### Check Runner Status
```bash
systemctl --user status gitea-runner
```

### View Logs
```bash
journalctl --user -f -u gitea-runner
```

### Check Gitea UI
Go to: **Admin → Runners Management**
Should see your host runner with status **Active** and labels `ubuntu-latest, host`

---

## 🎯 Runner Architecture

### Current Setup:

| Runner | Type | Location | Labels | Purpose |
|--------|------|----------|--------|---------|
| Container (ID: 6) | Container | docker-compose | ubuntu-latest, ubuntu-24.04 | Main CI/CD |
| Host | Native | ~/gitea-runner | ubuntu-latest, host | Hardware access |

### Why Two Runners?

1. **Container Runner**: Sandboxed environment, clean builds
2. **Host Runner**: Direct hardware access (USB devices, serial ports)

---

## 🔧 Management

### Start/Stop
```bash
systemctl --user start gitea-runner
systemctl --user stop gitea-runner
systemctl --user restart gitea-runner
```

### Logs
```bash
# Live logs
journalctl --user -f -u gitea-runner

# Last 100 lines
journalctl --user -n 100 -u gitea-runner

# Since last boot
journalctl --user -b -u gitea-runner
```

### Remove Runner

From Gitea UI:
1. Admin → Runners Management
2. Find your runner
3. Click Edit → Delete

From host:
```bash
systemctl --user stop gitea-runner
rm -rf ~/gitea-runner
```

---

## 🐛 Troubleshooting

### Runner doesn't appear in Gitea UI

1. Check logs:
   ```bash
   journalctl --user -f -u gitea-runner
   ```

2. Verify Gitea connectivity:
   ```bash
   curl -I http://localhost:3000
   ```

3. Verify token validity (regenerate if needed in Gitea UI)

### Jobs not being picked up

Check runner labels match workflow `runs-on`:
```bash
journalctl --user -n 50 -u gitea-runner | grep labels
```

### Service won't start

Check for port conflicts or permission issues:
```bash
journalctl --user -xe -u gitea-runner
```

### Connection refused

Verify Gitea is running:
```bash
podman ps | grep gitea
curl http://localhost:3000/api/v1/version
```

---

## 📝 Testing

Проект использует `.gitea/workflows/simple-ci.yml`. Пример отдельного тестового workflow:

```yaml
name: Test Both Runners

on:
  workflow_dispatch:

jobs:
  on-container:
    name: Container Runner
    runs-on: ubuntu-latest
    steps:
      - run: echo "Running on container runner"
      - run: uname -a

  on-host:
    name: Host Runner
    runs-on: host
    steps:
      - run: echo "Running on host runner"
      - run: uname -a
      - run: whoami
```

Push and trigger from Gitea UI → Actions
