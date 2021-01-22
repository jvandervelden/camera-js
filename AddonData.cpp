#ifndef __addondata_cpp__
#define __addondata_cpp__

#include <node.h>
#include "camera.h"

using namespace v8;

class AddonData
{
public:
    AddonData(Isolate *isolate, Local<Object> exports) : camera(new Camera()) {
        camera->Init();
        // Link the existence of this object instance to the existence of exports.
        exports_.Reset(isolate, exports);
        exports_.SetWeak(this, DeleteMe, WeakCallbackType::kParameter);
    }

    ~AddonData() {
        if (!exports_.IsEmpty()) {
            // Reset the reference to avoid leaking data.
            exports_.ClearWeak();
            exports_.Reset();
        }
    }

    // Per-addon data.
    Camera* camera;

private:
    // Method to call when "exports" is about to be garbage-collected.
    static void DeleteMe(const WeakCallbackInfo<AddonData> &info) {
        info.GetParameter()->camera->Stop();
        delete info.GetParameter()->camera;
        delete info.GetParameter();
    }

    // Weak handle to the "exports" object. An instance of this class will be
    // destroyed along with the exports object to which it is weakly bound.
    v8::Persistent<v8::Object> exports_;
};

#endif