FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

# ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# # Optionally install the cmake for vcpkg
# COPY ./reinstall-cmake.sh /tmp/

# RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
#         chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
#     fi \
#     && rm -f /tmp/reinstall-cmake.sh

RUN apt update && export DEBIAN_FRONTEND=noninteractive \
    && apt -y install --no-install-recommends \
       cmake \
       vim \
       libboost-all-dev \
       clang-format \
       ccache \
       clang \
       gdb

RUN git clone https://github.com/google/googletest.git -b v1.16.0 \
    && cd googletest \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make \
    && make install \
    && cd ../.. \
    && rm -rf googletest
