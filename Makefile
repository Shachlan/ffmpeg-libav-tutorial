VIDEO_URL := http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

hello_world: clean small_bunny_1080p_60fps.mp4
	gcc -g -Wall -o build/hello_world -lavformat -lavcodec -lswscale -lz 0_hello_world.c \
	  && ./build/hello_world $(lastword $?)

transcoding: clean
	gcc -g -Wall -o build/transcoding -lavformat -lavcodec -lswscale -lz -lavutil -L ./OpenGL/target/debug -lopenglrust -framework OpenGL 2_transcoding.c preparation.c rationalExtensions.c \
		&& echo "done building" \
	  && LD_LIBRARY_PATH=./OpenGL/target/debug:$LD_LIBRARY_PATH ./build/transcoding dog.mp4 small_bunny_1080p_60fps.mp4 bunny_1s_gop.mp4 0.8 1 

clean:
	rm -f ./build/hello_world ./build/transcoding
