VIDEO_URL := http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

transcoding: clean
	mkdir -p ./build && \
	cp ./src/opengl/shaders/* ./build/
	g++ -g -Wall -std=c++14 ./src/all.hpp -o ./build/all.hpp.gch \
	&& g++ -g -std=c++14 -Wall -o build/transcoding \
	-lboost_date_time -lavformat -lavcodec -lswscale -lz -lglfw -lavutil -framework OpenGL \
	./src/*.cpp ./src/opengl/*.cpp ./src/transcoding/*.cpp \
	 -I./src/ -I./src/opengl/ -I./src/transcoding/ -I ./build -include all.hpp -stdlib=libc++ -DGL_SILENCE_DEPRECATION=1  &&\
	 cd ./build &&\
	 ./transcoding ./../movies/small_bunny_1080p_60fps.mp4 ./../movies/dog.mp4  ./../movies/bunny_1s_gop.mp4 0.5 1 4

clean:
	rm -rf ./build/*
