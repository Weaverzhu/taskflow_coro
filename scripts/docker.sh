#!/bin/bash

docker build . -f ./docker/Dockerfile.ubuntu --build-arg http_proxy=$http_proxy \
--build-arg https_proxy=$https_proxy \
-t test