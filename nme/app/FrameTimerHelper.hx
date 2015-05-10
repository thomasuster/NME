package nme.app;
class FrameTimerHelper {

    public var framePeriod:Float;

    var render:Bool;
    var last:Float;

    public function new():Void {
    }


    public function tick(timestamp:Float):Void {
        if(render)
            render = false;
        if(timestamp - last >= framePeriod) {
            render = !render;
        }
        last = timestamp;
    }

    public function shouldRender():Bool {
        return render;
    }
}

