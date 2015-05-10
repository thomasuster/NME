package nme.app;
class TestFrameTimerHelper extends haxe.unit.TestCase
{
    var frameTimer:FrameTimerHelper;

    override public function setup():Void {
        frameTimer = new FrameTimerHelper();
        frameTimer.framePeriod = 100;
    }

    public function testInitialCase() {
        frameTimer.tick(1234);
        assertTrue(frameTimer.shouldRender());
    }

    public function testSameTimeTick() {
        frameTimer.tick(1234);
        frameTimer.tick(1234);
        assertFalse(frameTimer.shouldRender());
    }

    public function testFarEnoughTick() {
        frameTimer.tick(1234);
        frameTimer.tick(1234+100);
        assertTrue(frameTimer.shouldRender());
    }

    public function testFarEnoughTickWithTickInBetween() {
        frameTimer.tick(1234);
        frameTimer.tick(1234+50);
        frameTimer.tick(1234+100);
        assertTrue(frameTimer.shouldRender());
    }
}
