FROM ubuntu:24.04@sha256:1e0a86e57d247923571b75e0aaf48a1449cf8c543d51fb3e07a4a7d7bfa79316

LABEL org.opencontainers.image.source="https://github.com/lebedenko/rpi-dashboard"
LABEL org.opencontainers.image.description="Reproducible CI toolchain for rpi-dashboard"

ARG DEBIAN_FRONTEND=noninteractive
ARG CMAKE_VERSION=3.31.6
ARG NINJA_VERSION=1.13.2
ARG QT_VERSION=6.8.3
ARG AQTINSTALL_VERSION=3.3.0

RUN apt-get update \
  && apt-get install --no-install-recommends --yes \
  bash \
  ca-certificates \
  clang-tidy-18 \
  curl \
  g++-13 \
  gcc-13 \
  git \
  libdbus-1-3 \
  libegl1 \
  libfontconfig1 \
  libfreetype6 \
  libglib2.0-0 \
  libgl1 \
  libgl-dev \
  libtomlplusplus-dev \
  libx11-6 \
  libx11-xcb1 \
  libxcb-cursor0 \
  libxcb-icccm4 \
  libxcb-keysyms1 \
  libxcb-shape0 \
  libxcb-xinerama0 \
  libxkbcommon-x11-0 \
  python3 \
  python3-pip \
  unzip \
  xz-utils \
  && rm -rf /var/lib/apt/lists/*

RUN curl --fail --location --show-error \
  "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
  | tar --extract --gzip --directory=/opt \
  && ln -s "/opt/cmake-${CMAKE_VERSION}-linux-x86_64/bin/cmake" /usr/local/bin/cmake \
  && ln -s "/opt/cmake-${CMAKE_VERSION}-linux-x86_64/bin/ctest" /usr/local/bin/ctest

RUN curl --fail --location --show-error --output /tmp/ninja.zip \
  "https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip" \
  && unzip /tmp/ninja.zip -d /usr/local/bin \
  && chmod 0755 /usr/local/bin/ninja \
  && rm /tmp/ninja.zip

RUN python3 -m pip install --break-system-packages --no-cache-dir "aqtinstall==${AQTINSTALL_VERSION}" \
  && aqt install-qt linux desktop "${QT_VERSION}" linux_gcc_64 \
  --outputdir /opt/Qt

ENV CC=/usr/bin/gcc-13 \
  CXX=/usr/bin/g++-13 \
  CLANG_TIDY=/usr/bin/clang-tidy-18 \
  CMAKE_PREFIX_PATH=/opt/Qt/6.8.3/gcc_64 \
  LANG=C.UTF-8 \
  PATH=/opt/Qt/6.8.3/gcc_64/bin:/usr/local/bin:/usr/bin:/bin \
  QT_QPA_PLATFORM=offscreen \
  QT_QUICK_BACKEND=software

COPY ci/verify-toolchain.sh /usr/local/bin/verify-ci-toolchain
RUN ln -s /usr/bin/clang-tidy-18 /usr/local/bin/clang-tidy \
  && chmod 0755 /usr/local/bin/verify-ci-toolchain \
  && verify-ci-toolchain

WORKDIR /workspace
