cmd_Release/obj.target/camera.node := g++ -shared -pthread -rdynamic -m64  -Wl,-soname=camera.node -o Release/obj.target/camera.node -Wl,--start-group Release/obj.target/camera/node-addon.o -Wl,--end-group 