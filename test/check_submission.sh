#!/usr/bin/bash

set -e

if [[ ! $# -eq 1 ]]; then
    echo "Missing argument."
    echo "Usage: '$0 praxis{1,2,3}'"
    exit 1
fi

TEST_SCRIPT="test/test_$1.py"

if [[ ! -e ${TEST_SCRIPT} ]]; then
    echo "Requested test file ${TEST_SCRIPT} does not exist."
    exit 1
fi

PACKAGING_DIR=$(mktemp -d)
SOURCE_DIR=$(mktemp -d)
BUILD_DIR=$(mktemp -d)

echo "Packaging submission"
cmake -B${PACKAGING_DIR}
cmake --build ${PACKAGING_DIR} -t package_source

echo "Extracting and building submission"
(cd ${SOURCE_DIR} && find ${PACKAGING_DIR} -name '*.tar.gz' | head -n1 | xargs tar -xzf)
cmake -S"$(find ${SOURCE_DIR} -maxdepth 1 -mindepth 1 -type d | head -n1)" -B${BUILD_DIR}
cmake --build ${BUILD_DIR} -t webserver

echo "Executing tests"
pytest --executable ${BUILD_DIR}/webserver ${TEST_SCRIPT}
