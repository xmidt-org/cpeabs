#!/bin/bash

mkdir -p subprojects/packagefiles

pushd wrapdb

tar -czf rtMessage.tar.gz rtMessage
mv -f rtMessage.tar.gz ../subprojects/packagefiles/.

tar -czf rbuscore.tar.gz rbuscore
mv -f rbuscore.tar.gz ../subprojects/packagefiles/.

tar -czf rbus.tar.gz rbus
mv -f rbus.tar.gz ../subprojects/packagefiles/.

popd
