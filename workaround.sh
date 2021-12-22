#!/bin/bash

mkdir -p subprojects/packagefiles

pushd wrapdb

tar -czf rtMessage.tar.gz rtMessage
mv -f rtMessage.tar.gz ../subprojects/packagefiles/.

tar -czf rbuscore.tar.gz rbuscore
mv -f rbuscore.tar.gz ../subprojects/packagefiles/.

tar -czf rbus.tar.gz rbus
mv -f rbus.tar.gz ../subprojects/packagefiles/.

tar -czf cimplog.tar.gz cimplog
mv -f cimplog.tar.gz ../subprojects/packagefiles/.

tar -czf wdmp.tar.gz wdmp
mv -f wdmp.tar.gz ../subprojects/packagefiles/.

tar -czf jansson.tar.gz jansson
mv -f jansson.tar.gz ../subprojects/packagefiles/.

popd
