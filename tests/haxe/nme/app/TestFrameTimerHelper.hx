package nme.app;
class TestFrameTimerHelper extends haxe.unit.TestCase
{
    public function testInitialCase() {
        var frameTimer:FrameTimerHelper = new FrameTimerHelper();
        frameTimer.tick(1234);
        assertTrue(frameTimer.shouldRender);
    }
}
