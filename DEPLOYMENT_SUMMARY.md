# CryptoWallet Infrastructure Deployment Summary

## ✅ Status

**Gitea** - ✅ **Fully Operational**
- Version: 1.25.5
- Database: SQLite3 (persistent on host)
- Admin User: `admin` / `pass`
- Web UI: http://localhost:3000
- SSH: localhost:2222

**Runner** - ⏸️ **Paused** (can be enabled later)

## 📁 Directory Structure

```
/data/gitea/
├── data/                    # Bind mount for Gitea data
│   ├── gitea/
│   │   ├── gitea.db        # SQLite database (PERSISTENT)
│   │   ├── conf/
│   │   │   └── app.ini     # Configuration
│   │   └── ... other files
│   ├── git/
│   │   └── .ssh/           # SSH keys
│   └── ssh/                # SSH host keys
└── repositories/            # Bind mount for Git repositories
    └── pilgrim/
        └── cryptowallet.git # Your repository (PERSISTENT)
```

## 🚀 Quick Start

### Start Infrastructure

```bash
cd /data/projects/CryptoWallet

# Start with environment file
podman-compose -f infra/docker-compose.yml --env-file infra/.env.local up -d

# Or manually start Gitea only
podman run -d --name cryptowallet-gitea \
  --network infra_dev-network \
  -p 3000:3000 -p 2222:22 \
  -v /data/gitea/data:/data \
  -v /data/gitea/repositories:/data/git/repositories \
  -e GITEA__SECURITY__INSTALL_LOCK=true \
  -e GITEA__SERVER__DOMAIN=localhost \
  -e GITEA__SERVER__ROOT_URL=http://localhost:3000/ \
  -e GITEA__SERVER__SSH_PORT=2222 \
  docker.io/gitea/gitea:latest
```

### Access Gitea

- **Web UI**: http://localhost:3000
- **Admin User**: `admin` / `pass`
- **SSH**: `ssh://git@localhost:2222/pilgrim/cryptowallet.git`

## 💾 Data Persistence

### What Gets Saved

| Data | Location | Mount Point | Type |
|------|----------|-------------|------|
| Database | /data/gitea/data/gitea/gitea.db | /data (in container) | SQLite |
| Configuration | /data/gitea/data/gitea/conf/ | /data (in container) | Files |
| SSH Keys | /data/gitea/data/ssh/ | /data (in container) | Files |
| Repositories | /data/gitea/repositories/ | /data/git/repositories | Bind Mount |

### Verification

After any restart, verify persistence:

```bash
# Check admin user exists
podman exec cryptowallet-gitea sqlite3 /data/gitea/gitea.db \
  "SELECT lower_name, email FROM user WHERE is_admin = 1;"

# Check database is on host
ls -lh /data/gitea/data/gitea/gitea.db

# Check repository exists
du -sh /data/gitea/repositories/pilgrim/cryptowallet.git

# Check API works
curl http://localhost:3000/api/v1/version
```

## 🔐 Credentials & Tokens

### Admin User
- Username: `admin`
- Password: `pass`
- Email: `admin@localhost`

### Runner Token (if needed later)
- Stored in: `/data/projects/CryptoWallet/infra/.env.local`
- Format: `GITEA_RUNNER_TOKEN=<token>`

## 📝 Important Files

- **docker-compose.yml**: `/data/projects/CryptoWallet/infra/docker-compose.yml`
- **Environment**: `/data/projects/CryptoWallet/infra/.env.local`
- **Configuration**: `/data/projects/CryptoWallet/infra/.env`

## 🔧 Common Operations

### Stop Gitea
```bash
podman stop cryptowallet-gitea
```

### Start Gitea
```bash
podman start cryptowallet-gitea
```

### View Logs
```bash
podman logs -f cryptowallet-gitea
```

### Access Database
```bash
podman exec cryptowallet-gitea sqlite3 /data/gitea/gitea.db
```

### Restart
```bash
podman restart cryptowallet-gitea
```

## ⚠️ Notes

1. **Database Locks**: If making direct DB changes, ensure Gitea is stopped
2. **Permissions**: Host directories are owned by git user (uid 100999) in container
3. **Network**: Both containers use `infra_dev-network` bridge network
4. **SSH Port**: External port 2222 maps to internal port 22

## 📌 Next Steps

1. ✅ Gitea fully operational and persistent
2. ⏸️ Runner registration needs debugging (skipped for now)
3. 📦 Ready for:
   - Creating organizations/teams
   - Setting up CI/CD workflows
   - Adding collaborators
   - Creating webhooks

## 🐛 Troubleshooting

**Gitea not starting**: Check logs with `podman logs cryptowallet-gitea`

**Can't access web UI**: Ensure port 3000 is not blocked, check with `netstat -tln | grep 3000`

**SSH connection refused**: Check port 2222 is mapped correctly, use `ssh://git@localhost:2222/...`

**Database locked**: Stop Gitea before making direct DB changes

---

**Last Updated**: 2026-03-21  
**Status**: Production Ready (Gitea only, Runner pending)
