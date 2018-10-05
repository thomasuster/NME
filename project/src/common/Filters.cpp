#include <Graphics.h>
#include <Display.h>
#include <Surface.h>
#include <nme/Pixel.h>

namespace nme
{


Surface *ExtractAlpha(const Surface *inSurface)
{
   int w =  inSurface->Width();
   int h = inSurface->Height();
   Surface *result = new SimpleSurface(w,h,pfAlpha);
   result->IncRef();

   AutoSurfaceRender render(result);
   inSurface->BlitChannel(render.Target(), Rect(0,0,w,h), 0, 0, CHAN_ALPHA, CHAN_ALPHA );
   return result;
}

/*
 
   The BlurFilter has its size, mBlurX, mBlurY.  These are the "extra" pixels
    that get blended together. So Blur of 0 = original image, 1 = 1 extra, 2 = 2 extra.

   The even blends are easy: central pixel + Blur/2 either size.

   The Odd blends takes *source* pixels from the *right* first, at quality 1, and then from
    the left first for the second quality pass.  This means the destination will be
    bigger on the *left* first.  (flip the convolution filter)

*/



// --- BlurFilter --------------------------------------------------------------

BlurFilter::BlurFilter(int inQuality, int inBlurX, int inBlurY) : Filter(inQuality)
{
   mBlurX = std::max(0, std::min(256, inBlurX-1) );
   mBlurY = std::max(0, std::min(256, inBlurY-1) );
}

void BlurFilter::ExpandVisibleFilterDomain(Rect &ioRect,int inPass) const
{
   // This is about the source rect, so we have to add pixels to the right first,
   //  from where they will be taken first.
   int extra_x0 = mBlurX/2;
   int extra_x1 = mBlurX - extra_x0;
   int extra_y0 = mBlurY/2;
   int extra_y1 = mBlurY - extra_y0;

   if (inPass & 1)
   {
      std::swap(extra_x0, extra_x1);
      std::swap(extra_y0, extra_y1);
   }

   ioRect.x -= extra_x0;
   ioRect.y -= extra_y0;
   ioRect.w += mBlurX;
   ioRect.h += mBlurY;

}

void BlurFilter::GetFilteredObjectRect(Rect &ioRect,int inPass) const
{
   // Distination pixels can "move" more left, as these left pixels can take extra from the right
   int extra_x1 = mBlurX/2;
   int extra_x0 = mBlurX - extra_x1;
   int extra_y1 = mBlurY/2;
   int extra_y0 = mBlurY - extra_y1;

   if (inPass & 1)
   {
      std::swap(extra_x0, extra_x1);
      std::swap(extra_y0, extra_y1);
   }

   ioRect.x -= extra_x0;
   ioRect.y -= extra_y0;
   ioRect.w += mBlurX;
   ioRect.h += mBlurY;
}

/*
   Mask of size 4 looks like:  x+xx where + is the centre
   The blurreed image is then 2 pixel bigger in the left and one on the right
*/

/*
  inSrc        - src pixel corresponding to first output pixel
  inDS         - pixel stride
  inSrcW       - number of valid source pixels after inSrc
  inFilterLeft - filter size on left

  inDest       - first output pixel
  inDD         - output pixel stride
  inDest       - number of pixels to render
  
  inFilterSize - total filter size
  inPixelsLeft - number of valid pixels on left
*/
template<bool PREM>
void BlurRow(const BGRA<PREM> *inSrc, int inDS, int inSrcW, int inFilterLeft,
             BGRA<PREM> *inDest, int inDD, int inDestW, int inFilterSize,int inPixelsLeft)
{
   typedef BGRA<PREM> Pixel;
   int sr = 0;
   int sg = 0;
   int sb = 0;
   int sa = 0;

   // loop over destination pixels with kernel    -xxx+
   // At each pixel, we - the trailing pixel and + the leading pixel
   const Pixel *prev = inSrc - inFilterLeft*inDS;
   const Pixel *first = std::max(prev,inSrc - inPixelsLeft*inDS);
   const Pixel *src = prev + inFilterSize*inDS;
   const Pixel *src_end = inSrc + inSrcW*inDS;
   Pixel *dest = inDest;
   for(const Pixel *s=first;s<src;s+=inDS)
   {
      int a = s->a;
      sa+=a;
      sr+= s->getRAlpha();
      sg+= s->getGAlpha();
      sb+= s->getBAlpha();
   }
   for(int x=0;x<inDestW; x++)
   {
      if (prev>=src_end)
      {
         for( ; x<inDestW; x++ )
         {
            dest->ival = 0;
            dest+=inDD;
         }
         return;
      }

      if (sa==0)
         dest->ival = 0;
      else if (PREM)
      {
         dest->r = sr/inFilterSize;
         dest->g = sg/inFilterSize;
         dest->b = sb/inFilterSize;
         dest->a = sa/inFilterSize;
      }
      else
      {
         dest->r = (sr*255)/sa;
         dest->g = (sg*255)/sa;
         dest->b = (sb*255)/sa;
         dest->a = sa/inFilterSize;
      }

      if (src>=inSrc && src<src_end)
      {
         int a = src->a;
         sa+=a;
         sr+= src->getRAlpha();
         sg+= src->getGAlpha();
         sb+= src->getBAlpha();
      }

      if (prev>=first)
      {
         int a = prev->a;
         sa-=a;
         sr-= prev->getRAlpha();
         sg-= prev->getGAlpha();
         sb-= prev->getBAlpha();
      }


      src+=inDS;
      prev+=inDS;
      dest+=inDD;
   }
}

    
// Alpha version
void BlurRow(const uint8 *inSrc, int inDS, int inSrcW, int inFilterLeft,
             uint8 *inDest, int inDD, int inDestW, int inFilterSize,int inPixelsLeft)
{
   int sa = 0;

   // loop over destination pixels with kernel    -xxx+
   // At each pixel, we - the trailing pixel and + the leading pixel
   const uint8 *prev = inSrc - inFilterLeft*inDS;
   const uint8 *first = std::max(prev,inSrc - inPixelsLeft*inDS);
   const uint8 *src = prev + inFilterSize*inDS;
   const uint8 *src_end = inSrc + inSrcW*inDS;
   uint8 *dest = inDest;
   for(const uint8 *s=first;s<src;s+=inDS)
      sa += *s;

   for(int x=0;x<inDestW; x++)
   {
      if (prev>=src_end)
      {
         for( ; x<inDestW; x++ )
         {
            *dest = 0;
            dest+=inDD;
         }
         return;
      }

      //wtf
      switch(inFilterSize) {
         case 1: *dest = sa / 1; break;
         case 2: *dest = sa / 2; break;
         case 3: *dest = sa / 3; break;
         case 4: *dest = sa / 4; break;
         case 5: *dest = sa / 5; break;
         case 6: *dest = sa / 6; break;
         case 7: *dest = sa / 7; break;
         case 8: *dest = sa / 8; break;
         case 9: *dest = sa / 9; break;
         case 10: *dest = sa / 10; break;
         case 11: *dest = sa / 11; break;
         case 12: *dest = sa / 12; break;
         case 13: *dest = sa / 13; break;
         case 14: *dest = sa / 14; break;
         case 15: *dest = sa / 15; break;
         case 16: *dest = sa / 16; break;
         case 17: *dest = sa / 17; break;
         case 18: *dest = sa / 18; break;
         case 19: *dest = sa / 19; break;
         case 20: *dest = sa / 20; break;
         case 21: *dest = sa / 21; break;
         case 22: *dest = sa / 22; break;
         case 23: *dest = sa / 23; break;
         case 24: *dest = sa / 24; break;
         case 25: *dest = sa / 25; break;
         case 26: *dest = sa / 26; break;
         case 27: *dest = sa / 27; break;
         case 28: *dest = sa / 28; break;
         case 29: *dest = sa / 29; break;
         case 30: *dest = sa / 30; break;
         case 31: *dest = sa / 31; break;
         case 32: *dest = sa / 32; break;
         case 33: *dest = sa / 33; break;
         case 34: *dest = sa / 34; break;
         case 35: *dest = sa / 35; break;
         case 36: *dest = sa / 36; break;
         case 37: *dest = sa / 37; break;
         case 38: *dest = sa / 38; break;
         case 39: *dest = sa / 39; break;
         case 40: *dest = sa / 40; break;
         case 41: *dest = sa / 41; break;
         case 42: *dest = sa / 42; break;
         case 43: *dest = sa / 43; break;
         case 44: *dest = sa / 44; break;
         case 45: *dest = sa / 45; break;
         case 46: *dest = sa / 46; break;
         case 47: *dest = sa / 47; break;
         case 48: *dest = sa / 48; break;
         case 49: *dest = sa / 49; break;
         case 50: *dest = sa / 50; break;
         case 51: *dest = sa / 51; break;
         case 52: *dest = sa / 52; break;
         case 53: *dest = sa / 53; break;
         case 54: *dest = sa / 54; break;
         case 55: *dest = sa / 55; break;
         case 56: *dest = sa / 56; break;
         case 57: *dest = sa / 57; break;
         case 58: *dest = sa / 58; break;
         case 59: *dest = sa / 59; break;
         case 60: *dest = sa / 60; break;
         case 61: *dest = sa / 61; break;
         case 62: *dest = sa / 62; break;
         case 63: *dest = sa / 63; break;
         case 64: *dest = sa / 64; break;
         case 65: *dest = sa / 65; break;
         case 66: *dest = sa / 66; break;
         case 67: *dest = sa / 67; break;
         case 68: *dest = sa / 68; break;
         case 69: *dest = sa / 69; break;
         case 70: *dest = sa / 70; break;
         case 71: *dest = sa / 71; break;
         case 72: *dest = sa / 72; break;
         case 73: *dest = sa / 73; break;
         case 74: *dest = sa / 74; break;
         case 75: *dest = sa / 75; break;
         case 76: *dest = sa / 76; break;
         case 77: *dest = sa / 77; break;
         case 78: *dest = sa / 78; break;
         case 79: *dest = sa / 79; break;
         case 80: *dest = sa / 80; break;
         case 81: *dest = sa / 81; break;
         case 82: *dest = sa / 82; break;
         case 83: *dest = sa / 83; break;
         case 84: *dest = sa / 84; break;
         case 85: *dest = sa / 85; break;
         case 86: *dest = sa / 86; break;
         case 87: *dest = sa / 87; break;
         case 88: *dest = sa / 88; break;
         case 89: *dest = sa / 89; break;
         case 90: *dest = sa / 90; break;
         case 91: *dest = sa / 91; break;
         case 92: *dest = sa / 92; break;
         case 93: *dest = sa / 93; break;
         case 94: *dest = sa / 94; break;
         case 95: *dest = sa / 95; break;
         case 96: *dest = sa / 96; break;
         case 97: *dest = sa / 97; break;
         case 98: *dest = sa / 98; break;
         case 99: *dest = sa / 99; break;
         case 100: *dest = sa / 100; break;
         case 101: *dest = sa / 101; break;
         case 102: *dest = sa / 102; break;
         case 103: *dest = sa / 103; break;
         case 104: *dest = sa / 104; break;
         case 105: *dest = sa / 105; break;
         case 106: *dest = sa / 106; break;
         case 107: *dest = sa / 107; break;
         case 108: *dest = sa / 108; break;
         case 109: *dest = sa / 109; break;
         case 110: *dest = sa / 110; break;
         case 111: *dest = sa / 111; break;
         case 112: *dest = sa / 112; break;
         case 113: *dest = sa / 113; break;
         case 114: *dest = sa / 114; break;
         case 115: *dest = sa / 115; break;
         case 116: *dest = sa / 116; break;
         case 117: *dest = sa / 117; break;
         case 118: *dest = sa / 118; break;
         case 119: *dest = sa / 119; break;
         case 120: *dest = sa / 120; break;
         case 121: *dest = sa / 121; break;
         case 122: *dest = sa / 122; break;
         case 123: *dest = sa / 123; break;
         case 124: *dest = sa / 124; break;
         case 125: *dest = sa / 125; break;
         case 126: *dest = sa / 126; break;
         case 127: *dest = sa / 127; break;
         case 128: *dest = sa / 128; break;
         case 129: *dest = sa / 129; break;
         case 130: *dest = sa / 130; break;
         case 131: *dest = sa / 131; break;
         case 132: *dest = sa / 132; break;
         case 133: *dest = sa / 133; break;
         case 134: *dest = sa / 134; break;
         case 135: *dest = sa / 135; break;
         case 136: *dest = sa / 136; break;
         case 137: *dest = sa / 137; break;
         case 138: *dest = sa / 138; break;
         case 139: *dest = sa / 139; break;
         case 140: *dest = sa / 140; break;
         case 141: *dest = sa / 141; break;
         case 142: *dest = sa / 142; break;
         case 143: *dest = sa / 143; break;
         case 144: *dest = sa / 144; break;
         case 145: *dest = sa / 145; break;
         case 146: *dest = sa / 146; break;
         case 147: *dest = sa / 147; break;
         case 148: *dest = sa / 148; break;
         case 149: *dest = sa / 149; break;
         case 150: *dest = sa / 150; break;
         case 151: *dest = sa / 151; break;
         case 152: *dest = sa / 152; break;
         case 153: *dest = sa / 153; break;
         case 154: *dest = sa / 154; break;
         case 155: *dest = sa / 155; break;
         case 156: *dest = sa / 156; break;
         case 157: *dest = sa / 157; break;
         case 158: *dest = sa / 158; break;
         case 159: *dest = sa / 159; break;
         case 160: *dest = sa / 160; break;
         case 161: *dest = sa / 161; break;
         case 162: *dest = sa / 162; break;
         case 163: *dest = sa / 163; break;
         case 164: *dest = sa / 164; break;
         case 165: *dest = sa / 165; break;
         case 166: *dest = sa / 166; break;
         case 167: *dest = sa / 167; break;
         case 168: *dest = sa / 168; break;
         case 169: *dest = sa / 169; break;
         case 170: *dest = sa / 170; break;
         case 171: *dest = sa / 171; break;
         case 172: *dest = sa / 172; break;
         case 173: *dest = sa / 173; break;
         case 174: *dest = sa / 174; break;
         case 175: *dest = sa / 175; break;
         case 176: *dest = sa / 176; break;
         case 177: *dest = sa / 177; break;
         case 178: *dest = sa / 178; break;
         case 179: *dest = sa / 179; break;
         case 180: *dest = sa / 180; break;
         case 181: *dest = sa / 181; break;
         case 182: *dest = sa / 182; break;
         case 183: *dest = sa / 183; break;
         case 184: *dest = sa / 184; break;
         case 185: *dest = sa / 185; break;
         case 186: *dest = sa / 186; break;
         case 187: *dest = sa / 187; break;
         case 188: *dest = sa / 188; break;
         case 189: *dest = sa / 189; break;
         case 190: *dest = sa / 190; break;
         case 191: *dest = sa / 191; break;
         case 192: *dest = sa / 192; break;
         case 193: *dest = sa / 193; break;
         case 194: *dest = sa / 194; break;
         case 195: *dest = sa / 195; break;
         case 196: *dest = sa / 196; break;
         case 197: *dest = sa / 197; break;
         case 198: *dest = sa / 198; break;
         case 199: *dest = sa / 199; break;
         case 200: *dest = sa / 200; break;
         case 201: *dest = sa / 201; break;
         case 202: *dest = sa / 202; break;
         case 203: *dest = sa / 203; break;
         case 204: *dest = sa / 204; break;
         case 205: *dest = sa / 205; break;
         case 206: *dest = sa / 206; break;
         case 207: *dest = sa / 207; break;
         case 208: *dest = sa / 208; break;
         case 209: *dest = sa / 209; break;
         case 210: *dest = sa / 210; break;
         case 211: *dest = sa / 211; break;
         case 212: *dest = sa / 212; break;
         case 213: *dest = sa / 213; break;
         case 214: *dest = sa / 214; break;
         case 215: *dest = sa / 215; break;
         case 216: *dest = sa / 216; break;
         case 217: *dest = sa / 217; break;
         case 218: *dest = sa / 218; break;
         case 219: *dest = sa / 219; break;
         case 220: *dest = sa / 220; break;
         case 221: *dest = sa / 221; break;
         case 222: *dest = sa / 222; break;
         case 223: *dest = sa / 223; break;
         case 224: *dest = sa / 224; break;
         case 225: *dest = sa / 225; break;
         case 226: *dest = sa / 226; break;
         case 227: *dest = sa / 227; break;
         case 228: *dest = sa / 228; break;
         case 229: *dest = sa / 229; break;
         case 230: *dest = sa / 230; break;
         case 231: *dest = sa / 231; break;
         case 232: *dest = sa / 232; break;
         case 233: *dest = sa / 233; break;
         case 234: *dest = sa / 234; break;
         case 235: *dest = sa / 235; break;
         case 236: *dest = sa / 236; break;
         case 237: *dest = sa / 237; break;
         case 238: *dest = sa / 238; break;
         case 239: *dest = sa / 239; break;
         case 240: *dest = sa / 240; break;
         case 241: *dest = sa / 241; break;
         case 242: *dest = sa / 242; break;
         case 243: *dest = sa / 243; break;
         case 244: *dest = sa / 244; break;
         case 245: *dest = sa / 245; break;
         case 246: *dest = sa / 246; break;
         case 247: *dest = sa / 247; break;
         case 248: *dest = sa / 248; break;
         case 249: *dest = sa / 249; break;
         case 250: *dest = sa / 250; break;
         case 251: *dest = sa / 251; break;
         case 252: *dest = sa / 252; break;
         case 253: *dest = sa / 253; break;
         case 254: *dest = sa / 254; break;
         case 255: *dest = sa / 255; break;
      }

      if (src>=inSrc && src<src_end)
         sa+=*src;

      if (prev>=first)
         sa -= *prev;

      src+=inDS;
      prev+=inDS;
      dest+=inDD;
   }
}



template<typename PIXEL>
void BlurFilter::DoApply(const Surface *inSrc,Surface *outDest,ImagePoint inSrc0,ImagePoint inDiff,int inPass
      ) const
{
   int w = outDest->Width();
   int h = outDest->Height();
   int sw = inSrc->Width();
   int sh = inSrc->Height();

   int blurred_w = std::min(sw+mBlurX,w);
   int blurred_h = std::min(sh+mBlurY,h);
   // TODO: tmp height is potentially less (h+mBlurY) than sh ...
   SimpleSurface *tmp = new SimpleSurface(blurred_w,sh,outDest->Format());
   tmp->IncRef();

   int ox = mBlurX/2;
   int oy = mBlurY/2;
   if ( (inPass & 1) == 0)
   {
      ox = mBlurX - ox;
      oy = mBlurY - oy;
   }

   {
   AutoSurfaceRender tmp_render(tmp);
   const RenderTarget &target = tmp_render.Target();
   // Blur rows ...
   int sx0 = inSrc0.x + inDiff.x;
   for(int y=0;y<sh;y++)
   {
      PIXEL *dest = (PIXEL *)target.Row(y);
      const PIXEL *src = ((PIXEL *)inSrc->Row(y)) + sx0;

      BlurRow(src,1,sw-sx0,ox, dest,1,blurred_w, mBlurX+1, sx0);
   }
   sw = tmp->Width();
   }

   AutoSurfaceRender dest_render(outDest);
   const RenderTarget &target = dest_render.Target();
   int s_stride = tmp->GetStride()/sizeof(PIXEL);
   int d_stride = target.mSoftStride/sizeof(PIXEL);
   // Blur cols ...
   int sy0 = inSrc0.y + inDiff.y;
   for(int x=0;x<blurred_w;x++)
   {
      PIXEL *dest = (PIXEL *)target.Row(0) + x;
      const PIXEL *src = ((PIXEL *)tmp->Row(sy0)) + x;

      BlurRow(src,s_stride,sh-sy0,oy, dest,d_stride,blurred_h,mBlurY+1,sy0);
   }

   tmp->DecRef();
}

void BlurFilter::Apply(const Surface *inSrc,Surface *outDest,ImagePoint inSrc0,ImagePoint inDiff,int inPass) const
{
   PixelFormat sFmt = inSrc->Format();
   PixelFormat dFmt = outDest->Format();

   if (sFmt==pfBGRPremA && dFmt==pfBGRPremA)
      DoApply<BGRPremA>(inSrc,outDest,inSrc0,inDiff,inPass);
   else if (sFmt==pfBGRA && dFmt==pfBGRA)
      DoApply<ARGB>(inSrc,outDest,inSrc0,inDiff,inPass);

   //ApplyStrength(mStrength,outDest);
}


// --- ColorMatrixFilter -------------------------------------------------------------

ColorMatrixFilter::ColorMatrixFilter(QuickVec<float> inMatrix) : Filter(1)
{
   mMatrix = inMatrix;
}

void ColorMatrixFilter::ExpandVisibleFilterDomain(Rect &ioRect,int inPass) const
{
}

void ColorMatrixFilter::GetFilteredObjectRect(Rect &ioRect,int inPass) const
{
}

int clamp255 (float val)
{
   if (val > 0xff) return 0xff;
   else if (val < 0) return 0;
   else return int (val);
}

template<typename PIXEL>
void ColorMatrixFilter::DoApply(const Surface *inSrc,Surface *outDest,ImagePoint inSrc0,ImagePoint inDiff,int inPass
      ) const
{
   int w = outDest->Width();
   int h = outDest->Height();
   int sw = inSrc->Width();
   int sh = inSrc->Height();
   
   //outDest->Zero();
   
   int filter_w = std::min(sw,w);
   int filter_h = std::min(sh,h);
   
   AutoSurfaceRender render(outDest);
   const RenderTarget &target = render.Target();
   for(int y=0;y<filter_h;y++)
   {
      ARGB *src = (ARGB*)inSrc->Row(y);
      ARGB *dest = (ARGB*)target.Row(y);
      for(int x=0;x<filter_w;x++)
      {
         dest->r = clamp255 ((mMatrix[0]  * src->r) + (mMatrix[1]  * src->g) + (mMatrix[2]  * src->b) + (mMatrix[3]  * src->a) + mMatrix[4]);
         dest->g = clamp255 ((mMatrix[5]  * src->r) + (mMatrix[6]  * src->g) + (mMatrix[7]  * src->b) + (mMatrix[8]  * src->a) + mMatrix[9]);
         dest->b = clamp255 ((mMatrix[10]  * src->r) + (mMatrix[11]  * src->g) + (mMatrix[12]  * src->b) + (mMatrix[13]  * src->a) + mMatrix[14]);
         dest->a = clamp255 ((mMatrix[15]  * src->r) + (mMatrix[16]  * src->g) + (mMatrix[17]  * src->b) + (mMatrix[18]  * src->a) + mMatrix[19]);
         src++;
         dest++;
      }
   }
}

void ColorMatrixFilter::Apply(const Surface *inSrc,Surface *outDest,ImagePoint inSrc0,ImagePoint inDiff,int inPass) const
{
   DoApply<ARGB>(inSrc,outDest,inSrc0,inDiff,inPass);
   //ApplyStrength(mStrength,outDest);
}


// --- DropShadowFilter --------------------------------------------------------------

DropShadowFilter::DropShadowFilter(int inQuality, int inBlurX, int inBlurY,
      double inTheta, double inDistance, int inColour, double inStrength,
      double inAlpha, bool inHide, bool inKnockout, bool inInner )
  : BlurFilter(inQuality, inBlurX, inBlurY),
     mCol(inColour), mAlpha(inAlpha),
     mHideObject(inHide), mKnockout(inKnockout), mInner(inInner)
{
   double theta = inTheta * M_PI/180.0;
   if (inDistance>255) inDistance = 255;
   if (inDistance<0) inDistance = 0;
   mTX = (int)( cos(theta) * inDistance );
   mTY = (int)( sin(theta) * inDistance );

   mStrength  = (int)(inStrength* 256);
   if ((unsigned int)mStrength>0x10000)
      mStrength = 0x10000;

   mAlpha  = (int)(inAlpha*256);
   if ((unsigned int)mAlpha > 256) mAlpha = 256;

   mAlpha255  = (int)(inAlpha*255);
   if ((unsigned int)mAlpha255 > 255) mAlpha255 = 255;

}

void ShadowRect(const RenderTarget &inTarget, const Rect &inRect, int inCol,int inStrength)
{
   Rect rect = inTarget.mRect.Intersect(inRect);
   int a = ((inCol >> 24 ) + (inCol>>31))*inStrength>>8;
   int r = (inCol>>16) & 0xff;
   int g = (inCol>>8) & 0xff;
   int b = inCol & 0xff;
   for(int y=0;y<rect.h;y++)
   {
      ARGB *argb = ( (ARGB *)inTarget.Row(y+rect.y)) + rect.x;
      for(int x=0;x<rect.w;x++)
      {
         argb->r += ((r-argb->r)*a)>>8;
         argb->g += ((g-argb->g)*a)>>8;
         argb->b += ((b-argb->b)*a)>>8;
         argb++;
      }
   }
}


void ApplyStrength(Surface *inAlpha,int inStrength)
{
   if (inStrength!=0x100)
   {
      uint8 lut[256];
      for(int a=0;a<256;a++)
      {
         int v= (a*inStrength) >> 8;
         lut[a] = v<255 ? v : 255;
      }
      AutoSurfaceRender render(inAlpha);
      const RenderTarget &target = render.Target();
      int w = target.Width();
      for(int y=0;y<target.Height();y++)
      {
         if (inAlpha->Format()==pfAlpha)
         {
            uint8 *r = target.Row(y);
            for(int x=0;x<w;x++)
               r[x] = lut[r[x]];
         }
         else
         {
            ARGB *r = (ARGB*)target.Row(y);
            for(int x=0;x<w;x++)
               r[x].a = lut[r[x].a];
         }
      }
   }
}


/*
void DumpAlpha(const char *inName, const Surface *inSurf)
{
   printf("------ %s ------\n", inName);
   for(int i=0;i<12;i++)
   {
      for(int x=0;x<12;x++)
         printf( inSurf->Row(i)[x]>128 ? "X" : ".");
      printf("\n");
   }
   printf("\n");
}
*/


void DropShadowFilter::Apply(const Surface *inSrc,Surface *outDest,ImagePoint inSrc0,ImagePoint inDiff,int inPass) const
{
   bool inner_hide = mInner && ( mKnockout || mHideObject);
   Surface *alpha = ExtractAlpha(inSrc);
   Surface *orig_alpha = 0;
   if (inner_hide)
      orig_alpha = alpha->IncRef();

   // Blur alpha..
   ImagePoint offset(0,0);
   ImagePoint a_src(inSrc0);
   for(int q=0;q<mQuality;q++)
   {
      Rect src_rect(alpha->Width(),alpha->Height());
      BlurFilter::GetFilteredObjectRect(src_rect,q);
      Surface *blur = new SimpleSurface(src_rect.w, src_rect.h, pfAlpha);
      blur->IncRef();

      ImagePoint diff(src_rect.x, src_rect.y);

      DoApply<uint8>(alpha,blur,a_src,diff,q);

      a_src = ImagePoint(0,0);
      alpha->DecRef();
      alpha = blur;
      offset += diff;
   }

   ApplyStrength(alpha,mStrength);


   AutoSurfaceRender render(outDest);
	outDest->Zero();
   //outDest->Clear(0xff00ff00);
   const RenderTarget &target = render.Target();

   // Copy it into the destination rect...
   ImagePoint blur_pos = offset + ImagePoint(mTX,mTY) - inDiff;
   int a = mAlpha;

   int scol = mCol;
   Rect src(inSrc0.x,inSrc0.y,inSrc->Width(),inSrc->Height());

   if (mInner )
   {
      if (a>127) a--;
      scol = (scol & 0xffffff) | (a<<24);
      if (inner_hide)
      {
         orig_alpha->BlitTo(target, src, -inDiff.x, -inDiff.y, bmTinted, 0, scol );
      }
      else
      {
         inSrc->BlitTo(target, src, -inDiff.x, -inDiff.y , bmCopy, 0, 0xffffff );
      }

      // And overlay shadow...
      Rect rect(alpha->Width(), alpha->Height() );
      alpha->BlitTo(target, rect, blur_pos.x, blur_pos.y, inner_hide ? bmErase : bmTintedInner, 0, scol);

      if (!inner_hide)
      {
         // Missing overlap between blurred and object...
         ImagePoint obj_pos = offset;
         int all = 999999;

         if (blur_pos.x > obj_pos.x)
            ShadowRect(target,Rect(obj_pos.x, blur_pos.y, blur_pos.x-obj_pos.x, rect.h), scol, mStrength);

         if (blur_pos.y > obj_pos.y)
            ShadowRect(target,Rect(obj_pos.x, obj_pos.y, all, blur_pos.y-obj_pos.y), scol, mStrength);
   
         if (blur_pos.x+rect.w < outDest->Width())
            ShadowRect(target,Rect(blur_pos.x+rect.w, blur_pos.y, all, rect.h), scol, mStrength);

         if (blur_pos.y+rect.h < outDest->Height())
            ShadowRect(target,Rect(obj_pos.x, blur_pos.y + rect.h, all, all), scol, mStrength);
      }
   }
   else
   {

      int dy0 = std::max(0,blur_pos.y);
      int dy1 = std::min(outDest->Height(),blur_pos.y+alpha->Height());
      int dx0 = std::max(0,blur_pos.x);
      int dx1 = std::min(outDest->Width(),blur_pos.x+alpha->Width());

      if (dx1>dx0)
      {
         int col = scol & 0x00ffffff;
         for(int y=dy0;y<dy1;y++)
         {
            ARGB *dest = ((ARGB *)target.Row(y)) + dx0;
            const uint8 *src = alpha->Row(y-blur_pos.y) + dx0 - blur_pos.x;
            for(int x=dx0;x<dx1;x++)
            {
               dest++->ival = col | ( (((*src++)*a)>>8) << 24 );
            }
         }
      }

      if (mKnockout)
      {
         inSrc->BlitTo(target, src, -inDiff.x, -inDiff.y, bmErase, 0, 0xffffff );
      }
      else if (!mHideObject)
      {
         inSrc->BlitTo(target, src, -inDiff.x, -inDiff.y, bmNormal, 0, 0xffffff );
      }
   }
   
   alpha->DecRef();
   if (orig_alpha)
      orig_alpha->DecRef();

}

void DropShadowFilter::ExpandVisibleFilterDomain(Rect &ioRect,int inPass) const
{
   Rect orig = ioRect;

   // Handle the quality ourselves, so iterate here.
   // Work out blur component...
   for(int q=0;q<mQuality;q++)
      BlurFilter::ExpandVisibleFilterDomain(ioRect,q);

   ioRect.Translate(-mTX,-mTY);

   if (!mKnockout)
      ioRect = ioRect.Union(orig);
}

void DropShadowFilter::GetFilteredObjectRect(Rect &ioRect,int inPass) const
{
   Rect orig = ioRect;

   if (!mInner)
   {
      // Handle the quality ourselves, so iterate here.
      // Work out blur component...
      for(int q=0;q<mQuality;q++)
         BlurFilter::GetFilteredObjectRect(ioRect,q);

      ioRect.Translate(mTX,mTY);

      if (!mKnockout && !mHideObject)
         ioRect = ioRect.Union(orig);

      //ioRect.x--;
      //ioRect.y--;
      //ioRect.w+=2;
      //ioRect.h+=2;
   }
}



// --- FilterList --------------------------------------------------------------


Rect ExpandVisibleFilterDomain( const FilterList &inList, const Rect &inRect )
{
   Rect r = inRect;
   for(int i=0;i<inList.size();i++)
   {
      Filter *f = inList[i];
      int quality = f->GetQuality();
      for(int q=0;q<quality;q++)
         f->ExpandVisibleFilterDomain(r, q);
   }
   return r;
}

// Given the intial pixel rect, calculate the filtered pixels...
Rect GetFilteredObjectRect( const FilterList &inList, const Rect &inRect)
{
   Rect r = inRect;
   for(int i=0;i<inList.size();i++)
   {
      Filter *f = inList[i];
      int quality = f->GetQuality();
      for(int q=0;q<quality;q++)
         f->GetFilteredObjectRect(r, q);
   }
   return r;
}

void HighlightZeroAlpha(Surface *ioBMP)
{
   AutoSurfaceRender render(ioBMP);
   const RenderTarget &target = render.Target();

   for(int y=0;y<target.Height();y++)
   {
      ARGB *pixel = (ARGB *)target.Row(y);
      for(int x=0; x<target.Width(); x++)
      {
         if (pixel[x].a==0)
            pixel[x] = 0xff00ff00;
      }
   }
}


Surface *FilterBitmap( const FilterList &inFilters, Surface *inBitmap,
                       const Rect &inSrcRect, const Rect &inDestRect,
                       bool inMakePOW2, bool inRecycle,
                       ImagePoint inSrc0)
{
   int n = inFilters.size();
   PixelFormat fmt = inBitmap->Format();
   if (n==0 || (fmt!=pfBGRPremA && fmt!=pfBGRA) )
      return inBitmap;

   Rect src_rect = inSrcRect;

   Surface *bmp = inBitmap;

   if (fmt!=pfBGRA)
   {
      if (inRecycle)
         bmp->ChangeInternalFormat(pfBGRA);
      else
      {
         int w = bmp->Width();
         int h = bmp->Height();
         Surface *converted = new SimpleSurface(w,h,pfBGRA);
         converted->IncRef();
         PixelConvert(w,h, bmp->Format(), bmp->Row(0), bmp->GetStride(), 0,
                           pfBGRA, converted->EditRect(0,0,w,h), converted->GetStride(), 0 );
         bmp->DecRef();
         bmp = converted;
      }
   }

   bool do_clear = false;
   for(int i=0;i<n;i++)
   {
      Filter *f = inFilters[i];

      int quality = f->GetQuality();
      for(int q=0;q<quality;q++)
      {
         Rect dest_rect(src_rect);
         if (i==n-1 && q==quality-1)
         {
            dest_rect = inDestRect;
            if (inMakePOW2)
            {
              do_clear = true;
              dest_rect.w = UpToPower2(dest_rect.w);
              dest_rect.h = UpToPower2(dest_rect.h);
            }
         }
         else
         {
            f->GetFilteredObjectRect(dest_rect, q);
         }

         Surface *filtered = new SimpleSurface(dest_rect.w,dest_rect.h,bmp->Format());
         filtered->IncRef();

         if (do_clear)
            filtered->Zero();

         f->Apply(bmp,filtered, inSrc0, ImagePoint(dest_rect.x-src_rect.x, dest_rect.y-src_rect.y), q );
         inSrc0 = ImagePoint(0,0);

         bmp->DecRef();
         bmp = filtered;
         src_rect = dest_rect;
      }
   }

   //HighlightZeroAlpha(bmp);

   if (fmt==pfBGRPremA)
      bmp->ChangeInternalFormat(pfBGRPremA);

   return bmp;
}

 
} // end namespace nme


