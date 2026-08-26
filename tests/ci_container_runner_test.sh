#!/bin/sh

set -eu

runner=$1
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-ci-runner-test-XXXXXX")
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM

fake_docker=$test_dir/docker
arguments=$test_dir/arguments
lock_file=$test_dir/image.env
digest=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef

cat >"$fake_docker" <<'EOF'
#!/bin/sh
printf '%s\n' "$@" >"$TEST_DOCKER_ARGUMENTS"
for argument do
    case "$argument" in
        type=bind,src=*,dst=/workspace,readonly)
            source_mount=${argument#type=bind,src=}
            source_mount=${source_mount%,dst=/workspace,readonly}
            [ -d "$source_mount/build" ] || exit 125
            ;;
    esac
done
exit "${TEST_DOCKER_STATUS:-0}"
EOF
chmod +x "$fake_docker"

expect_status() {
    expected=$1
    shift
    set +e
    "$@" >/dev/null 2>&1
    actual=$?
    set -e
    [ "$actual" -eq "$expected" ] || {
        echo "ci_container_runner_test: expected status $expected, got $actual" >&2
        exit 1
    }
}

printf 'CI_IMAGE=ghcr.io/lebedenko/rpi-dashboard-ci@sha256:%s\n' "$digest" >"$lock_file"
TEST_DOCKER_ARGUMENTS=$arguments CI_IMAGE_LOCK_FILE=$lock_file DOCKER_COMMAND=$fake_docker \
    "$runner" dev >/dev/null
grep -Fx "ghcr.io/lebedenko/rpi-dashboard-ci@sha256:$digest" "$arguments" >/dev/null
grep -Fx 'dev' "$arguments" >/dev/null

clean_checkout=$test_dir/clean-checkout
mkdir -p "$clean_checkout/ci" "$clean_checkout/scripts"
cp "$runner" "$clean_checkout/scripts/run-ci-container.sh"
printf 'CI_IMAGE=ghcr.io/lebedenko/rpi-dashboard-ci@sha256:%s\n' "$digest" \
    >"$clean_checkout/ci/image.env"
TEST_DOCKER_ARGUMENTS=$arguments DOCKER_COMMAND=$fake_docker \
    "$clean_checkout/scripts/run-ci-container.sh" dev >/dev/null
[ ! -e "$clean_checkout/build" ] || {
    echo "ci_container_runner_test: temporary clean-checkout build mountpoint was not removed" >&2
    exit 1
}

printf 'CI_IMAGE=ghcr.io/lebedenko/rpi-dashboard-ci:latest\n' >"$lock_file"
expect_status 2 env TEST_DOCKER_ARGUMENTS=$arguments CI_IMAGE_LOCK_FILE=$lock_file \
    DOCKER_COMMAND=$fake_docker "$runner" dev

expect_status 127 env CI_IMAGE_LOCK_FILE=$lock_file DOCKER_COMMAND=definitely-missing-docker "$runner" dev
expect_status 2 env DOCKER_COMMAND=$fake_docker "$runner" release

TEST_DOCKER_ARGUMENTS=$arguments CI_IMAGE_OVERRIDE=local-ci-image DOCKER_COMMAND=$fake_docker \
    "$runner" tidy >/dev/null
grep -Fx 'local-ci-image' "$arguments" >/dev/null

expect_status 23 env TEST_DOCKER_ARGUMENTS=$arguments TEST_DOCKER_STATUS=23 \
    CI_IMAGE_OVERRIDE=local-ci-image DOCKER_COMMAND=$fake_docker "$runner" asan
