VIDEO_URL := http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

transcoding: clean
	g++ -g -Wall all.hpp -o ./build/all.hpp.gch \
	&& g++ -g -Wall -o build/transcoding -lavformat -lavcodec -lswscale -lz -lglfw -lavutil -framework OpenGL *.c *.cpp -I ./build -include all.hpp \
	  && ./build/transcoding ./movies/small_bunny_1080p_60fps.mp4 ./movies/dog.mp4  ./movies/bunny_1s_gop.mp4 0.8 1 4

clean:
	rm -rf ./build/*
