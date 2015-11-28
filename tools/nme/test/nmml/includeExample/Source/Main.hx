import nme.display.Sprite;
import nme.Assets;
class Main extends Sprite {

    public function new () {
        super ();
        if(Assets.getText('app/someTextFile.txt') == 'someTextFile.txt')
            Sys.exit(0);
        Sys.exit(1);
    }
}