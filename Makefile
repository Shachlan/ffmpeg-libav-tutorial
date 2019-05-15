VIDEO_URL := http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

transcoding: clean
	mkdir -p ./build2 && \
	cp ./src/opengl/shaders/* ./build2/
	g++ -g -Wall -std=c++17 ./src/all.hpp -o ./build2/all.hpp.gch \
	&& g++ -g -std=c++17 -Wall -o build2/transcoding \
	-lboost_date_time -lavformat -lavcodec -lswscale -lz -lglfw -lavutil -framework OpenGL \
	./src/*.cpp ./src/opengl/*.cpp ./src/transcoding/*.cpp \
	 -I./src/ -I./src/opengl/ -I./src/transcoding/ -I ./build2 -include all.hpp -stdlib=libc++ -DGL_SILENCE_DEPRECATION=1 -DDEBUG=1 &&\
	 cd ./build2 &&\
	 ./transcoding ./../movies/small_bunny_1080p_60fps.mp4 ./../movies/dog.mp4  ./../movies/bunny_1s_gop.mp4 0.5 1 4

clean:
	rm -rf ./build2/*
