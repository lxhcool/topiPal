#!/usr/bin/env bash
set -euo pipefail

IDENTITY_NAME="TopiPal Local Code Signing"
TMP_DIR="$(mktemp -d)"
KEY_PATH="$TMP_DIR/topi-codesign.key"
CERT_PATH="$TMP_DIR/topi-codesign.crt"
P12_PATH="$TMP_DIR/topi-codesign.p12"
KEYCHAIN="${HOME}/Library/Keychains/login.keychain-db"
P12_PASSWORD="topi-local-codesign"

cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

if security find-identity -v -p codesigning 2>/dev/null | grep -q "$IDENTITY_NAME"; then
    echo "$IDENTITY_NAME"
    exit 0
fi

openssl req \
    -new \
    -newkey rsa:2048 \
    -x509 \
    -days 3650 \
    -nodes \
    -subj "/CN=${IDENTITY_NAME}/" \
    -addext "extendedKeyUsage=codeSigning" \
    -keyout "$KEY_PATH" \
    -out "$CERT_PATH" >/dev/null 2>&1

openssl pkcs12 \
    -legacy \
    -export \
    -inkey "$KEY_PATH" \
    -in "$CERT_PATH" \
    -out "$P12_PATH" \
    -passout "pass:${P12_PASSWORD}" >/dev/null 2>&1

security import "$P12_PATH" \
    -k "$KEYCHAIN" \
    -P "$P12_PASSWORD" \
    -T /usr/bin/codesign >/dev/null

security add-trusted-cert \
    -d \
    -r trustRoot \
    -p codeSign \
    -k "$KEYCHAIN" \
    "$CERT_PATH" >/dev/null

echo "$IDENTITY_NAME"
