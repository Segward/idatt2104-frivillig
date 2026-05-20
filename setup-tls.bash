#!/usr/bin/env bash
# One-shot Let's Encrypt cert setup for segward.com.
#
# Run on the deployment box (Ubuntu) as a user with sudo.
#   sudo ./setup-tls.bash
#
# What it does:
#   1. Installs certbot.
#   2. Gets a cert for segward.com via the HTTP-01 challenge (needs port 80
#      reachable from the internet — make sure router forwards 80 -> this box).
#   3. Grants the current user read access to /etc/letsencrypt by adding it
#      to a new "ssl-cert-readers" group, so pm2 (running as that user) can
#      read fullchain.pem and privkey.pem.
#   4. Installs a deploy hook that restarts pm2 on every renewal so the
#      server picks up the new cert.

set -euo pipefail

DOMAIN="segward.com"
EMAIL="gustavhaverstad@gmail.com"
PM2_APP="crdt-server"
TARGET_USER="${SUDO_USER:-$USER}"
GROUP="ssl-cert-readers"

if [[ $EUID -ne 0 ]]; then
  echo "Please run with sudo: sudo $0" >&2
  exit 1
fi

echo "==> Installing certbot"
apt update
apt install -y certbot

echo "==> Stopping anything on port 80 (certbot needs it for HTTP-01)"
# Best-effort: ignore if nothing is listening.
fuser -k 80/tcp || true

echo "==> Requesting cert for ${DOMAIN}"
certbot certonly \
  --standalone \
  --non-interactive \
  --agree-tos \
  --email "${EMAIL}" \
  -d "${DOMAIN}"

echo "==> Granting ${TARGET_USER} read access to /etc/letsencrypt"
groupadd -f "${GROUP}"
usermod -aG "${GROUP}" "${TARGET_USER}"
chgrp -R "${GROUP}" /etc/letsencrypt/live /etc/letsencrypt/archive
chmod -R g+rX /etc/letsencrypt/live /etc/letsencrypt/archive

echo "==> Installing renewal deploy hook (restarts pm2 ${PM2_APP})"
HOOK=/etc/letsencrypt/renewal-hooks/deploy/restart-pm2.sh
mkdir -p "$(dirname "${HOOK}")"
cat > "${HOOK}" <<HOOK_EOF
#!/usr/bin/env bash
chgrp -R ${GROUP} /etc/letsencrypt/live /etc/letsencrypt/archive
chmod -R g+rX /etc/letsencrypt/live /etc/letsencrypt/archive
sudo -u ${TARGET_USER} pm2 restart ${PM2_APP} || true
HOOK_EOF
chmod +x "${HOOK}"

echo
echo "Done."
echo "Cert:  /etc/letsencrypt/live/${DOMAIN}/fullchain.pem"
echo "Key:   /etc/letsencrypt/live/${DOMAIN}/privkey.pem"
echo
echo "Log out and back in (or run 'newgrp ${GROUP}') so the group change"
echo "takes effect for ${TARGET_USER}, then rebuild and restart the server:"
echo "  cmake --build --preset default"
echo "  pm2 restart ${PM2_APP}"
