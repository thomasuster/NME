package;
import nme.app.TestFrameTimerHelper;
import haxe.Timer;
import nme.display.TestBitmapData;
class TestMain {

	static function main(){
        var r = new haxe.unit.TestRunner();
        r.add(new TestBitmapData());
        r.add(new TestFrameTimerHelper());
        var t0 = Timer.stamp();
        var success = r.run();
        trace(" Time : " + (Timer.stamp()-t0)*1000 );
        Sys.exit(success ? 0 : 1);
	}
}