# 🔐 Persistence Checklist for CryptoWallet Infrastructure

## ✅ Data Persistence Guarantee

### What Gets Saved Between Restarts

#### 1. **Gitea Database** ✅
- **Location:** `/data/gitea/gitea.db` (2.0M+)
- **Storage:** Podman volume `gitea_data:/data`
- **Contains:**
  - Users (admin, pilgrim, etc.)
  - Repositories metadata
  - SSH public keys
  - Actions/CI settings
  - Runner tokens

#### 2. **Configuration** ✅
- **app.ini:** `/data/gitea/conf/app.ini`
  - Contains: ROOT_URL, INSTALL_LOCK, Actions ENABLED
  - Storage: Podman volume
  
- **SSH Keys:** `/data/git/.ssh/authorized_keys`
  - Storage: Podman volume
  - Persists across restarts

#### 3. **Git Repositories** ✅
- **Location:** `/data/gitea/repositories` (host) → `/data/git/repositories` (container)
- **Storage:** Direct host bind mount
- **Contains:** All pushed code, commits, branches

#### 4. **Runner Configuration** ✅
- **Token:** Stored in gitea.db (after creating via UI)
- **Labels:** `linux/amd64,crypto,hil`
- **Capacity:** 1

---

## 🔄 Restart Procedure

### After `podman restart cryptowallet-gitea`:

```bash
# All volumes automatically remount
✅ gitea_data volume → /data/gitea/
✅ /data/gitea/repositories → /data/git/repositories
✅ SSH keys in /data/git/.ssh/

# All data is restored:
✅ Users (admin, pilgrim)
✅ Repositories and history
✅ Configurations
✅ SSH access keys
```

---

## ⚡ Verification Checklist

After any restart, verify with:

```bash
# 1. Containers running
podman ps | grep gitea

# 2. Database accessible
podman exec cryptowallet-gitea sqlite3 /data/gitea/gitea.db "SELECT COUNT(*) FROM user;"

# 3. Gitea API responding
curl http://localhost:3000/api/v1/version

# 4. SSH keys loaded
podman exec cryptowallet-gitea cat /data/git/.ssh/authorized_keys

# 5. Repositories accessible
ls -la /data/gitea/repositories/pilgrim/

# 6. Runner status
podman logs cryptowallet-runner | tail -20
```

---

## 📝 Important Notes

### Runner Token
- **Current Token:** `ZF3sW6UZwp5vLQ3kRk2LHrbJa5PO6yrSu4YgmqQW`
- **Status:** May be expired or invalid
- **Action Required:** Create new token via Gitea UI
  1. http://localhost:3000 → Settings → Actions → Runners
  2. Generate new token
  3. Update `infra/.env.local` and `infra/docker-compose.yml`

### Admin User
- **Username:** admin
- **Is Admin:** Yes (is_admin=1 in gitea.db)
- **Password:** Use Gitea UI to reset if needed

---

## 🎯 Conclusion

**All infrastructure data is 100% persistent!**

No data loss on:
- Container restart
- Host reboot
- Pod recreation

Everything persists through volumes and host mounts.
