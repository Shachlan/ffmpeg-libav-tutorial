VIDEO_URL := http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

hello_world: clean small_bunny_1080p_60fps.mp4
	gcc -g -Wall -o build/hello_world -lavformat -lavcodec -lswscale -lz 0_hello_world.c \
	  && ./build/hello_world $(lastword $?)

transcoding: clean
	g++ -g -Wall -o build/transcoding -lavformat -lavcodec -lswscale -lz -lglfw -lavutil -framework OpenGL transcoding.cpp openGLShading.cpp ffmpeg_wrappers.c rationalExtensions.c video-conversion-utilities.c \
	  && ./build/transcoding small_bunny_1080p_60fps.mp4 dog.mp4  bunny_1s_gop.mp4 0.8 1 4

clean:
	rm -f ./build/hello_world ./build/transcoding
