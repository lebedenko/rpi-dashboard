#!/bin/sh
set -eu
usage() { echo "Usage: $0 verify VERSION | dashboard VERSION OUTPUT | daemon VERSION ARCH BINARY OUTPUT" >&2; exit 2; }
stable_version() { printf '%s\n' "$1" | grep -Eq '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$'; }
project_version() { sed -n '/^project(RpiDashboard$/,/^)/s/^[[:space:]]*VERSION[[:space:]]*\([^[:space:]]*\).*$/\1/p' CMakeLists.txt; }
daemon_version() { sed -n 's/^project(DashboardDaemon VERSION \([^[:space:]]*\).*$/\1/p' daemon/CMakeLists.txt; }
verify() {
  version=$1
  stable_version "$version" || { echo "release: version must be stable MAJOR.MINOR.PATCH: $version" >&2; exit 2; }
  [ "$(project_version)" = "$version" ] || { echo "release: dashboard CMake version does not match $version" >&2; exit 1; }
  [ "$(daemon_version)" = "$version" ] || { echo "release: daemon CMake version does not match $version" >&2; exit 1; }
  grep -F "## [$version] - " CHANGELOG.md >/dev/null || { echo "release: CHANGELOG.md has no dated $version entry" >&2; exit 1; }
}
checksum() { (cd "$(dirname "$1")" && sha256sum "$(basename "$1")" >"$(basename "$1").sha256"); }
dashboard_archive() {
  version=$1 output=$2; verify "$version"; mkdir -p "$output"
  archive="$output/rpi-dashboard-$version.tar.gz"
  git archive --format=tar --prefix="rpi-dashboard-$version/" "${RELEASE_GIT_REF:-HEAD}" -- . ':(exclude)daemon' | gzip -n -9 >"$archive"
  checksum "$archive"
}
daemon_archive() {
  version=$1 arch=$2 binary=$3 output=$4; verify "$version"
  case "$arch" in x86_64|aarch64) ;; *) echo "release: unsupported daemon architecture: $arch" >&2; exit 2;; esac
  [ -x "$binary" ] || { echo "release: daemon binary is not executable: $binary" >&2; exit 1; }
  [ "$("$binary" --version)" = "dashboard-daemon $version" ] || { echo "release: daemon executable version mismatch" >&2; exit 1; }
  mkdir -p "$output"; work=$(mktemp -d "${TMPDIR:-/tmp}/dashboard-daemon-package-XXXXXX"); trap 'rm -rf -- "$work"' EXIT HUP INT TERM
  prefix="dashboard-daemon-$version-linux-$arch"; mkdir "$work/$prefix"
  cp "$binary" "$work/$prefix/dashboard-daemon"
  cp daemon/package/dashboard-daemon.service daemon/package/config.toml "$work/$prefix/"
  sed "s/@PACKAGE_ARCH@/$arch/g" daemon/package/install.sh >"$work/$prefix/install.sh"
  chmod 0755 "$work/$prefix/dashboard-daemon" "$work/$prefix/install.sh"
  archive="$output/$prefix.tar.gz"
  tar --sort=name --owner=0 --group=0 --numeric-owner --mtime='UTC 1970-01-01' -C "$work" -cf - "$prefix" | gzip -n -9 >"$archive"
  checksum "$archive"
}
[ "$#" -ge 2 ] || usage
command=$1 version=$2
case "$command" in
  verify) [ "$#" -eq 2 ] || usage; verify "$version" ;;
  archive|dashboard) [ "$#" -eq 3 ] || usage; dashboard_archive "$version" "$3" ;;
  daemon) [ "$#" -eq 5 ] || usage; daemon_archive "$version" "$3" "$4" "$5" ;;
  *) usage ;;
esac
