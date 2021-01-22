const cam = require('./build/Release/camera.node');

function ReadFrame() {
    return new Promise((resolve, reject) => {
        cam.ReadFrame((test) => {
            resolve(test.pixels);
        });
    });
}

module.exports = {
    StartCapture: cam.StartCapture,
    ReadFrame,
    EndCapture: cam.EndCapture,
};