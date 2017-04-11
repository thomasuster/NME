cd tools/nme
haxe compile.hxml
cd ../..
cd project
neko build.n
cd ..
cd tools/nme
haxelib run nme setup