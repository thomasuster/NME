package nme.app;

@:nativeProperty
class FrameTimer implements IPollClient
{
   public var fps(default,set):Float;
   public var window:Window;
   public var invalid:Bool;
   public var catchup:Bool;
   public var offTarget:Float;

    var helper:FrameTimerHelper;

   public function new(inWindow:Window, inFps:Float)
   {
      helper = new FrameTimerHelper();
      helper.error = 0.0005;
      fps = inFps;
      window = inWindow;
      invalid = false;
      catchup = true;
      offTarget = 0.0;
      Application.addPollClient(this,true);
   }

   function set_fps(inFps:Float)
   {
      fps = inFps;
      helper.framePeriod = fps > 0 ? 1.0/fps : 0.0;
      return inFps;
   }

   public function onPoll(timestamp:Float):Void
   {
      if (window.active)
      {
         var wasInvalid =invalid;
         invalid = false;
         helper.tick(timestamp);
         if(fps > 0 && helper.shouldRender())
            window.onNewFrame();
//         else if (wasInvalid)
//            window.onInvalidFrame();
      }
   }

   public function invalidate()
   {
      invalid = true;
   }

   public function getNextWake(defaultWake:Float,timestamp:Float):Float
   {
      if (!window.active)
         return defaultWake;

      if (invalid)
         return 0.0;

      if (helper.framePeriod==0.0)
         return defaultWake;

      var next = helper.last + helper.framePeriod - haxe.Timer.stamp();
      if (next < defaultWake)
         return next;

      return defaultWake;
   }

}

