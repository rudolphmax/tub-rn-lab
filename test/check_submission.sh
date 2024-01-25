#!/bin/bash

set -e

if [[ ! $# -eq 1 ]]; then
    echo "Missing argument."
    echo "Usage: '$0 praxis{1,2,3}'"
    exit 1
fi

TEST_SCRIPT="$(dirname $0)/test_$1.py"

if [[ ! -e ${TEST_SCRIPT} ]]; then
    echo "Requested test file ${TEST_SCRIPT} does not exist!"
    exit 1
fi

echo "Packaging submission..."
PACKAGING_DIR=$(mktemp -d)
cmake -B${PACKAGING_DIR}
cmake --build ${PACKAGING_DIR} -t package_source

echo "Extracting submission..."
SOURCE_DIR=$(mktemp -d)
(cd ${SOURCE_DIR} && find ${PACKAGING_DIR} -name '*.tar.gz' | head -n1 | xargs tar -xzf)
EXTRACTED_SOURCE_DIR=$(find ${SOURCE_DIR} -maxdepth 1 -mindepth 1 -type d | head -n1)

if [[ ! -e ${EXTRACTED_SOURCE_DIR}/group.txt ]]; then
    echo "Submission does not contain a group.txt!"
    exit 1
fi

echo "Building submission..."
BUILD_DIR=$(mktemp -d)
cmake -S"${EXTRACTED_SOURCE_DIR}" -B${BUILD_DIR}
cmake --build ${BUILD_DIR} -t webserver

echo "Executing tests..."
python3 -m pytest --executable ${BUILD_DIR}/webserver ${TEST_SCRIPT}
