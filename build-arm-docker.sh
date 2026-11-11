#!/usr/bin/env bash
# Host-side wrapper: run build-arm.sh inside the rdk-kirkstone container. The container provides the
# RDK ARM SDK and network access for the dependency and ut-core / ut-control clones, so nothing is built on the host.
#
# Usage: ./build-arm-docker.sh          build inside the container
#        ./build-arm-docker.sh clean    clean build-arm/ inside the container
#        ./build-arm-docker.sh deps     build the prebuilt test dependencies into arm-deps/sysroot/ (once)
set -e

SRC_DIR=$(cd "$(dirname "$0")" && pwd)
IMAGE=${IMAGE:-rdk-kirkstone:latest}

# The image entrypoint runs as root, creates a user from LOCAL_USER_ID/LOCAL_GROUP_ID, then runs the
# command as that user with `bash -c "$@"`. So do not pass --user, and pass the command as one string.
# Only the source tree is mounted, at the same path, so build-arm/ is shared with the host. The entrypoint
# symlinks $LOCAL_START_DIR/.ssh into its own $HOME, so point LOCAL_START_DIR at the source tree: mounting the
# host home would make it create a self-referencing link inside the host ~/.ssh.
# Host networking: the bridge network's DNS dropped about every other source download during the build.
# The container is kept after each run so it stays visible in Docker Desktop; it is created only if missing.
CONTAINER=${CONTAINER:-l4-rdk-build}
LOCAL_GROUP_ID="$(getent passwd "$(id -u)" | cut -d: -f4)"

if ! docker container inspect "${CONTAINER}" >/dev/null 2>&1; then
    docker image inspect "${IMAGE}" >/dev/null 2>&1 || { echo "ERROR: image ${IMAGE} not found locally." >&2; exit 1; }
    docker run -d --name "${CONTAINER}" --network=host \
        -e LOCAL_USER_ID="$(id -u)" \
        -e LOCAL_GROUP_ID="${LOCAL_GROUP_ID}" \
        -e LOCAL_START_DIR="${SRC_DIR}" \
        -v "${SRC_DIR}:${SRC_DIR}" \
        "${IMAGE}" "sleep infinity" >/dev/null
elif [ "$(docker inspect -f '{{.State.Running}}' "${CONTAINER}")" != "true" ]; then
    docker start "${CONTAINER}" >/dev/null
fi

# Builds run inside the running container, as the user the entrypoint created for this host user.
# The dependency build runs as root because it installs into the SDK sysroot. It hands the output back to the host user.
if [ "${1:-}" = "deps" ]; then
    shift
    docker exec -u root -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" -w "${SRC_DIR}" "${CONTAINER}" \
        ./arm-deps/build-deps.sh "$@"
else
    docker exec -u "$(id -u):${LOCAL_GROUP_ID}" -w "${SRC_DIR}" "${CONTAINER}" ./build-arm.sh "$@"
fi
