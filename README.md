# Camera JS

Camera addon for node js using video 4 linux.

Hardcoded at 320 x 240, and JPEG binary stream.

To use:

```js
const camera = require('camera-js');

camera.StartCapture();

const frameInfo = await camera.ReadFrame();

// Do some work with frameInfo.pixels

camera.EndCapture();
```
