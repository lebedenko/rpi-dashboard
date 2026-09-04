#!/bin/sh

set -eu

release_script=$1
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-release-test-XXXXXX")
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM
repo=$test_dir/repo
mkdir "$repo"
cp "$release_script" "$repo/release.sh"
printf '%s\n' 0.1.1 >"$repo/VERSION"
printf '%s\n' 'project(RpiDashboard VERSION 0.1.1)' >"$repo/CMakeLists.txt"
mkdir "$repo/daemon"
printf '%s\n' 'project(DashboardDaemon VERSION 0.1.1 LANGUAGES CXX)' >"$repo/daemon/CMakeLists.txt"
mkdir "$repo/daemon/package"
printf '%s\n' daemon-only >"$repo/daemon/package/secret"
printf '%s\n' 'host = ""' >"$repo/daemon/package/config.toml"
printf '%s\n' '[Service]' 'ExecStart=/usr/local/bin/dashboard-daemon' >"$repo/daemon/package/dashboard-daemon.service"
printf '%s\n' '#!/bin/sh' 'package_arch=@PACKAGE_ARCH@' >"$repo/daemon/package/install.sh"
printf '%s\n' '# Changelog' '' '## [0.1.1] - 2026-08-29' '' '- Split packages.' >"$repo/CHANGELOG.md"
(cd "$repo" && git init -q && git add . && git -c user.name=test -c user.email=test@example.invalid commit -qm test)

(cd "$repo" && sh release.sh verify 0.1.1)
if (cd "$repo" && sh release.sh verify 0.1 >/dev/null 2>&1); then
    echo "release_tool_test: accepted an unstable version" >&2
    exit 1
fi
(cd "$repo" && sh release.sh dashboard 0.1.1 "$test_dir/one")
(cd "$repo" && sh release.sh dashboard 0.1.1 "$test_dir/two")
cmp "$test_dir/one/rpi-dashboard-0.1.1.tar.gz" "$test_dir/two/rpi-dashboard-0.1.1.tar.gz"
(cd "$test_dir/one" && sha256sum --check rpi-dashboard-0.1.1.tar.gz.sha256)
manifest=$(tar -tzf "$test_dir/one/rpi-dashboard-0.1.1.tar.gz")
printf '%s\n' "$manifest" | grep -Fqx 'rpi-dashboard-0.1.1/CHANGELOG.md'
if printf '%s\n' "$manifest" | grep -q '^rpi-dashboard-0.1.1/daemon/'; then
    echo "release_tool_test: dashboard archive contains daemon files" >&2
    exit 1
fi
cat >"$repo/dashboard-daemon" <<'EOF'
#!/bin/sh
[ "$1" = --version ] && printf '%s\n' 'dashboard-daemon 0.1.1'
EOF
chmod +x "$repo/dashboard-daemon"
(cd "$repo" && sh release.sh daemon 0.1.1 x86_64 ./dashboard-daemon "$test_dir/daemon")
(cd "$test_dir/daemon" && sha256sum --check dashboard-daemon-0.1.1-linux-x86_64.tar.gz.sha256)
tar -tzf "$test_dir/daemon/dashboard-daemon-0.1.1-linux-x86_64.tar.gz" | LC_ALL=C sort >"$test_dir/daemon-manifest"
cat >"$test_dir/expected-daemon-manifest" <<'EOF'
dashboard-daemon-0.1.1-linux-x86_64/
dashboard-daemon-0.1.1-linux-x86_64/config.toml
dashboard-daemon-0.1.1-linux-x86_64/dashboard-daemon
dashboard-daemon-0.1.1-linux-x86_64/dashboard-daemon.service
dashboard-daemon-0.1.1-linux-x86_64/install.sh
EOF
cmp "$test_dir/expected-daemon-manifest" "$test_dir/daemon-manifest"

mkdir "$test_dir/failing-bin"
cat >"$test_dir/failing-bin/tar" <<'EOF'
#!/bin/sh
case " $* " in
  *' --sort=name '*) echo 'tar: unrecognized option: sort=name' >&2; exit 64 ;;
  *) exec "$REAL_TAR" "$@" ;;
esac
EOF
chmod +x "$test_dir/failing-bin/tar"
real_tar=$(command -v tar)
if (cd "$repo" && PATH="$test_dir/failing-bin:$PATH" REAL_TAR="$real_tar" \
    sh release.sh daemon 0.1.1 x86_64 ./dashboard-daemon "$test_dir/tar-failure"); then
    echo "release_tool_test: tar failure produced a successful empty archive" >&2
    exit 1
fi
[ ! -e "$test_dir/tar-failure/dashboard-daemon-0.1.1-linux-x86_64.tar.gz.sha256" ]
[ ! -e "$test_dir/tar-failure/dashboard-daemon-0.1.1-linux-x86_64.tar.gz" ]
