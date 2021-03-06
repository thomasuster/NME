#include <hx/CFFI.h>
#include <nme/Object.h>
#include <nme/NmeApi.h>
#include <nme/ImageBuffer.h>
#include <Camera.h>

#ifdef ANDROID
#include <android/log.h>
#endif

using namespace nme;

namespace nme
{
static int _id_on_error;
static int _id_init_frame;
static int _id_init_frame_fmt;
static int _id_on_frame;
static int _id_width;
static int _id_height;
static int _id_frameId;

using namespace nme;

void InitCamera()
{
    _id_on_error = val_id("_on_error");
    _id_init_frame = val_id("_init_frame");
    _id_init_frame_fmt = val_id("_init_frame_fmt");
    _id_on_frame = val_id("_on_frame");
    _id_width = val_id("width");
    _id_height = val_id("height");
    _id_frameId = val_id("frameId");
}


}


ImageBuffer *valueToImageBuffer(value inBmp)
{
   #ifndef HXCPP_JS_PRIME
   if (val_is_kind(inBmp,gObjectKind) )
   {
       Object *obj = (Object *)val_to_kind(inBmp,gObjectKind);
       if (obj)
          return obj->asImageBuffer();
   }
   #endif
   return 0;
}



// --- Camera --------------------

namespace nme
{

Camera::Camera() : status(camInit), buffer(0), width(0), height(0), pixelFormat(pfBGRA)
{
   frameId = 0;
}



bool Camera::setError(const std::string &inError)
{
   printf(" -> %s\n", inError.c_str() );
   error = inError;
   status = camError;
   return false;
}

FrameBuffer *Camera::getReadBuffer()
{
  lock();
  FrameBuffer *result = 0;
  for(int i=0;i<3;i++)
  {
     FrameBuffer &test = frameBuffers[i];
     if (test.age>=0)
     {
        if (result==0 || test.age>result->age)
           result = &test;
     }
  }
  unlock();
  return result;
}


FrameBuffer *Camera::getWriteBuffer()
{
  lock();
  FrameBuffer *result = 0;
  for(int i=0;i<3;i++)
  {
     FrameBuffer &test = frameBuffers[i];
     if (result==0 || test.age<result->age)
        result = &test;
  }
  result->age = -1;
  unlock();
  return result;
}


void Camera::syncUpdate(value handler)
{
   if (status==camError)
   {
      val_ocall1(handler, _id_on_error, alloc_string_len(error.c_str(), error.size()) );
   }
   else if (status==camRunning && !buffer)
   {
      alloc_field(handler, _id_width, alloc_int(width));
      alloc_field(handler, _id_height, alloc_int(height));
      value bmp = pixelFormat==pfBGRA ? val_ocall0(handler, _id_init_frame) :
                                        val_ocall1(handler, _id_init_frame_fmt, alloc_int(pixelFormat) );
      buffer = valueToImageBuffer(bmp);
      //printf("Got image buffer %p %p (%d)\n", bmp, buffer, buffer ? buffer->Format() : 0);
   }
}


void Camera::onPoll(value handler)
{
   syncUpdate(handler);

   if (status==camRunning && buffer)
   {
      lock();
      FrameBuffer *frameBuffer = getReadBuffer();
      unlock();

      if (frameBuffer)
      {
         copyFrame(buffer,frameBuffer);
         releaseFrameBuffer(frameBuffer);
         onFrame(handler);
      }
   }
}

void Camera::onFrame(value handler)
{
   val_ocall0(handler, _id_on_frame);
}

} // end namespace nme

value nme_camera_create(value inName)
{
   HxString name = valToHxString(inName);

   #if defined(__APPLE__) || defined(HX_WINDOWS) || defined(HX_LINUX)
   Camera *camera = CreateCamera(name.c_str());
   return ObjectToAbstract(camera);
   #else
   return alloc_null();
   #endif
}
DEFINE_PRIM(nme_camera_create,1);

value nme_camera_on_poll(value inCamera,value inHandler)
{
   Camera *camera;
   if (AbstractToObject(inCamera,camera))
      camera->onPoll(inHandler);
   return alloc_null();
}
DEFINE_PRIM(nme_camera_on_poll,2);





