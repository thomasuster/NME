package nme.app;
class FrameTimerHelper {

    public var framePeriod:Float;
    public var error:Float;

    var _shouldRender:Bool;
    var last:Float;

    public function new():Void {
    }


    public function tick(timestamp:Float):Void {
        if(_shouldRender)
            _shouldRender = false;
        if(timestamp - last + error >= framePeriod) {
            _shouldRender = true;
            last = timestamp;
        }
    }

    public function shouldRender():Bool {
        return _shouldRender;
    }
}

