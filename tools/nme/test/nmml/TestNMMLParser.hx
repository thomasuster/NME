package nmml;
import sys.io.Process;
class TestNMMLParser extends haxe.unit.TestCase
{
    public function testAssetRenames() {
        assertTrue(runExample('renameExample'));
    }

    public function testIncludedNMMLs() {
        assertTrue(runExample('includeExample'));
    }

    function runExample(name:String):Bool {
        var previous:String = Sys.getCwd();
        Sys.setCwd('test/nmml/$name');
        var process:Process = new Process('nme', ['test']);
        var all:String = process.stdout.readAll().toString();
        if(all != '')
            trace(all);
        var error:String = process.stderr.readAll().toString();
        if(error != '')
            trace(error);
        Sys.setCwd(previous);
        return process.exitCode() == 0;
    }
}
