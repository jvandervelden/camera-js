#include <math.h>

#include <node.h>

#include "camera.h"
#include "AddonData.cpp"

namespace demo {

	using v8::Context;
	using v8::Function;
	using v8::FunctionCallbackInfo;
	using v8::Isolate;
	using v8::Local;
	using v8::NewStringType;
	using v8::Name;
	using v8::Object;
	using v8::Number;
	using v8::String;
	using v8::Value;
	using v8::Array;
	using v8::ArrayBuffer;

	void StartCapture(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();
		Local<Context> context = isolate->GetCurrentContext();

		AddonData* data = reinterpret_cast<AddonData*>(args.Data().As<External>()->Value());
		Camera* camera = data->camera;

		if (camera->Start() != 0) {
			Local<String> errorString = String::NewFromUtf8(isolate, "Error Starting camera", NewStringType::kNormal).ToLocalChecked();
			args.GetReturnValue().Set(errorString);
		}
	}

	int clamp(uint n, uint low, uint high) {
    	if (n < low) { return low; }
    	if (n > high) { return high; }
    	return n;
	}

	void ReadFrameStream(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();
		Local<Context> context = isolate->GetCurrentContext();

		AddonData* data = reinterpret_cast<AddonData*>(args.Data().As<External>()->Value());
		Camera* camera = data->camera;

		Local<Function> cb = Local<Function>::Cast(args[0]);

		int res = camera->ReadFrame([camera, cb, context, isolate](void* data, uint size) -> int {
			Local<ArrayBuffer> buf = v8::ArrayBuffer::New(isolate, size);

			memcpy(buf->GetContents().Data(), data, size / 4);
			
			Local<Value> argv[1] = { buf };
			cb->Call(context, Null(isolate), 1, argv);
		});
	}

	void ReadFrame(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();
		Local<Context> context = isolate->GetCurrentContext();

		AddonData* data = reinterpret_cast<AddonData*>(args.Data().As<External>()->Value());
		Camera* camera = data->camera;

		Local<Function> cb = Local<Function>::Cast(args[0]);

		int res = camera->ReadFrame([camera, cb, context, isolate](void* data, uint size) -> int {
			uint ySize = camera->width * camera->height;
			uint cSize = ySize / 4;
			uint cbBaseIndex = ySize;
			uint crBaseIndex = ySize + cSize;

			char* pixels = (char*)data;
			Local<Array> pixelArray = Array::New(isolate, size);
			
			for (int i = 0; i < size; i++) {
				pixelArray->Set(i, Number::New(isolate, pixels[i]));
			}

			//Local<Array> pixelArray = Array::New(isolate, ySize);

			// for (int y = 0; y < camera->height; y++) {
            //     for (int x = 0; x < camera->width; x++) {
            //         int cX = floor(x / 2);
            //         int cY = floor(y / 2);
            //         int cIndex = cY * (camera->width / 2) + cX;
            //         int rgbIndex = y * camera->width + x;
                    
            //         char Y = pixels[rgbIndex];
            //         char Cb = pixels[cIndex + cbBaseIndex];
            //         char Cr = pixels[cIndex + crBaseIndex];
    
            //         int r = clamp(floor(Y + 1.4075 * (Cr - 128)), 0, 255); // << 24;
            //         int g = clamp(floor(Y - 0.3455 * (Cb - 128) - (0.7169 * (Cr - 128))), 0, 255); // << 16;
            //         int b = clamp(floor(Y + 1.7790 * (Cb - 128)), 0, 255); // << 8;
    
			// 		pixelArray->Set(rgbIndex * 4 + 0, Number::New(isolate, r));
			// 		pixelArray->Set(rgbIndex * 4 + 1, Number::New(isolate, g));
			// 		pixelArray->Set(rgbIndex * 4 + 2, Number::New(isolate, b));
			// 		pixelArray->Set(rgbIndex * 4 + 3, Number::New(isolate, 255));
            //     }
            // }

			Local<Object> imageInfo = Object::New(isolate);
			imageInfo->Set(String::NewFromUtf8(isolate, "pixels", NewStringType::kNormal).ToLocalChecked(), pixelArray);
			imageInfo->Set(String::NewFromUtf8(isolate, "width", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, camera->width));
			imageInfo->Set(String::NewFromUtf8(isolate, "height", NewStringType::kNormal).ToLocalChecked(), Number::New(isolate, camera->height));

			Local<Value> argv[1] = { imageInfo };
			cb->Call(context, Null(isolate), 1, argv);

			return 0;
		});

		if (res != 0) {
			Local<String> errorString = String::NewFromUtf8(isolate, "Error Reading Buffer", NewStringType::kNormal).ToLocalChecked();
			args.GetReturnValue().Set(errorString);
		}
	}

	void EndCapture(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();
		Local<Context> context = isolate->GetCurrentContext();

		AddonData* data = reinterpret_cast<AddonData*>(args.Data().As<External>()->Value());
		Camera* camera = data->camera;

		if (camera->Stop() != 0) {
			Local<String> errorString = String::NewFromUtf8(isolate, "Error Stopping camera", NewStringType::kNormal).ToLocalChecked();
			args.GetReturnValue().Set(errorString);
		}
	}
	        
	// Initialize this addon to be context-aware.
	NODE_MODULE_INIT(/* exports, module, context */) {
		Isolate* isolate = context->GetIsolate();

		// Create a new instance of AddonData for this instance of the addon.
		AddonData* data = new AddonData(isolate, exports);

		// Wrap the data in a v8::External so we can pass it to the method we expose.
		Local<External> external = External::New(isolate, data);

		// Expose the method "Method" to JavaScript, and make sure it receives the
		// per-addon-instance data we created above by passing `external` as the
		// third parameter to the FunctionTemplate constructor.
		exports->Set(context,
					String::NewFromUtf8(isolate, "ReadFrame", NewStringType::kNormal)
						.ToLocalChecked(),
					FunctionTemplate::New(isolate, ReadFrame, external)
						->GetFunction(context).ToLocalChecked()).FromJust();
		exports->Set(context,
					String::NewFromUtf8(isolate, "ReadFrameStream", NewStringType::kNormal)
						.ToLocalChecked(),
					FunctionTemplate::New(isolate, ReadFrameStream, external)
						->GetFunction(context).ToLocalChecked()).FromJust();
		exports->Set(context,
					String::NewFromUtf8(isolate, "StartCapture", NewStringType::kNormal)
						.ToLocalChecked(),
					FunctionTemplate::New(isolate, StartCapture, external)
						->GetFunction(context).ToLocalChecked()).FromJust();
		exports->Set(context,
					String::NewFromUtf8(isolate, "EndCapture", NewStringType::kNormal)
						.ToLocalChecked(),
					FunctionTemplate::New(isolate, EndCapture, external)
						->GetFunction(context).ToLocalChecked()).FromJust();
	}
}
