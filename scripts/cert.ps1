#!/usr/bin/env pwsh
param([string]$Domain = '')

$ErrorActionPreference = 'Stop'

New-Item -ItemType Directory -Force -Path .\certs | Out-Null

if ([string]::IsNullOrEmpty($Domain)) {
    openssl req -x509 -newkey rsa:2048 -nodes `
        -keyout .\certs\privkey.pem -out .\certs\fullchain.pem `
        -days 365 -subj "/CN=localhost"
    exit 0
}

$isAdmin = ([Security.Principal.WindowsPrincipal] `
    [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) { Write-Error "run as Administrator for domain certs" }

certbot certonly --standalone --non-interactive --agree-tos `
    --register-unsafely-without-email `
    --config-dir .\certs --work-dir .\certs\work --logs-dir .\certs\logs `
    -d $Domain --quiet

Copy-Item ".\certs\live\$Domain\fullchain.pem" .\certs\ -Force
Copy-Item ".\certs\live\$Domain\privkey.pem" .\certs\ -Force
