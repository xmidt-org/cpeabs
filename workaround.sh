#!/bin/bash

mkdir -p subprojects/packagefiles

pushd wrapdb
tar -czf rtMessage.tar.gz rtMessage
mv rtMessage.tar.gz ../subprojects/packagefiles/.
popd
