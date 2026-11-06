# Vendor Test Suite For AV pipeline

This document outlines the steps to build and execute the Vendor Test Suite (VTS) for AV pipeline, targeting Linux or ARM platforms.

## Table of Contents
- [Vendor Testing Suite For AV pipeline](#vendor-test-suite-for-av-pipeline)
  - [Acronyms, Terms and Abbreviations](#acronyms-terms-and-abbreviations)
  - [Setup](#setup)
  - [Setting up Docker](#setting-up-docker)
  - [Build AV VTS](#build-av-vts)
  - [Run Tests Manually](#run-tests-manually)

## Acronyms, Terms and Abbreviations

| Acronym / Term | Description |
|----------------|-------------|
| **AV** | Audio Video |
| **L4** | Level 4 Test Cases (Layer & System Testing) |
| **RDK** | Reference Design Kit |
| **UT-Core** | RDK Unified Test Core Framework |
| **VTS** | Vendor Test Suite |
| **YAML** | Yet Another Markup Language (configuration format) |

## Setup

### Configure Personal Access Token (PAT) for HTTPS access

The build scripts use HTTPS URLs to clone repositories. To avoid being prompted for credentials during the build, configure a GitHub Personal Access Token (PAT) in your `~/.netrc` file:

1. Generate a PAT on GitHub with `repo` (read) scope, preferably the `classic` one:
   - <https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens#personal-access-tokens-classic>

2. Add your credentials to `~/.netrc`:

```bash
echo "machine github.com login <your-github-username> password <your-PAT>" >> ~/.netrc

chmod 600 ~/.netrc
```

- Replace `<your-github-username>` with your GitHub username and `<your-PAT>` with the token generated above.
- The `chmod 600` restricts the file to owner-only access, which is required by Git.

### Configure SSH for GitHub (alternative)

- To avoid conflict between your organization GitHub and RDK Central GitHub, add the following in your ssh config (`~/.ssh/config`):

<!-- -->

    Host github.code.rdkcentral.com
      Hostname github.com
      User
      IdentityFile ~/.ssh/

- The instruction to generate SSH key for GitHub:

  - <https://docs.github.com/en/authentication/connecting-to-github-with-ssh/adding-a-new-ssh-key-to-your-github-account>

## Setting up Docker

### Installation

#### Requirements

- **The docker engine must be installed.** Installation instructions can be found https://docs.docker.com/get-docker/.
- **Access to the docker engine.** Access to the docker engine could require privileged permissions. On
Linux, it requires the user to be part of the 
`docker` group. See the installation instructions linked above
for more information about this.

#### Installing the docker image from .tar
Docker images received can be loaded into the docker engine with the below command:

```
docker load -i xxx.tar
```
This will allow new docker containers to be created from the docker image in the tar file. To validate the image
has been installed, use the following command to list all the installed images and check the new one is
present in the list.
```
docker image list -a
```

## Build AV VTS

1. Clone the Repository

```text
git clone https://github.com/rdkcentral/l4-rdk-audio-video.git

cd l4-rdk-audio-video

```
2. Install the host tools once
```bash
apt-get update && apt-get install -y --no-install-recommends \
    git curl patch xz-utils bzip2 cmake autoconf automake libtool gettext pkg-config qemu-user python3-pip m4 \
    && pip3 install meson==0.61.5 ninja

```
3. Build the dependency sysroot once. This will take some time.
```bash
bash ./arm-deps/build-deps.sh

```
4. Build the test framework and the device bundle.
```bash
bash ./build-arm.sh
```
Step 3 and step 4 clones UT Core and UT Control from GitHub, so the container needs stable network access.

## Run Tests Manually
- In order to run the test cases manually on target device, copy the tarball `l4-rdk-audio-video.tar.gz` to the target device.

```bash
docker cp <container_name>:/l4-rdk-audio-video/build-arm/device/l4-rdk-audio-video.tar.gz .
```
- Extract on Target Device. Log into the target device and extract the tarball:

Log into the target device and extract the tarball:
```bash
ssh root@target-device
cd /home/opt
tar -xzf l4-rdk-audio-video.tar.gz
```

Create a temporary folder to store test results and execute script.
```bash
mkdir -p /tmp/l4-results
/opt/l4-rdk-audio-video/run.sh -l /tmp/l4-results

```
The `/tmp/l4-results` directory must exist before the run. Without it, UT Core prints `Unable to open file for logging.`

The command runs every registered suite. 
Exit status `0` means UT Core reported success. `1` means the run finished with a failed test. `255` means UT Core could not start (`UT_init` failed).
