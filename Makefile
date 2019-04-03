VIDEO_URL := http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

hello_world: clean small_bunny_1080p_60fps.mp4
	gcc -g -Wall -o build/hello_world -lavformat -lavcodec -lswscale -lz 0_hello_world.c \
	  && ./build/hello_world $(lastword $?)

transcoding: clean
	gcc -g -Wall -o build/transcoding -lavformat -lavcodec -lswscale -lz -lglfw -framework OpenGL 2_transcoding.c openGLShading.c preparation.c \
	  && ./build/transcoding dog.mp4 small_bunny_1080p_60fps.mp4 bunny_1s_gop.mp4 0.3 10 

clean:
	rm -f ./build/hello_world ./build/transcoding
