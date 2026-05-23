#!/usr/bin/env sh
set -eu

mkdir -p ./certs
DOMAIN="${1:-}"

if [ -z "$DOMAIN" ]; then
  openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout ./certs/privkey.pem -out ./certs/fullchain.pem \
    -days 365 -subj "/CN=localhost"
  exit 0
fi

[ "$(id -u)" = 0 ] || { echo "run with sudo for domain certs" >&2; exit 1; }

apt-get install -y -qq certbot
fuser -k 80/tcp >/dev/null 2>&1 || true

certbot certonly --standalone --non-interactive --agree-tos \
  --register-unsafely-without-email \
  --config-dir ./certs --work-dir ./certs/work --logs-dir ./certs/logs \
  -d "$DOMAIN" --quiet

cp -L "./certs/live/$DOMAIN/fullchain.pem" ./certs/
cp -L "./certs/live/$DOMAIN/privkey.pem" ./certs/
chown -R "${SUDO_USER:-$USER}" ./certs
