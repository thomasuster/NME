set -e
cd ../../tools/nme
haxe compile.hxml
cd ../../project
neko Build.n mac
cd ../
cd tests/haxe
haxe compile.hxml
bin/TestMain
cd ../../../pakkaPets/bin
sh mac.sh -mute
cd ../../nme/tests/haxe