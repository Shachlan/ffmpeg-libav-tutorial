VIDEO_URL := http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

hello_world: clean small_bunny_1080p_60fps.mp4
	gcc -g -Wall -o build/hello_world -lavformat -lavcodec -lswscale -lz 0_hello_world.c \
	  && ./build/hello_world $(lastword $?)

./build/libskia.a: 
	cd submodules/skia/ &&\
	bin/gn gen out/skia/  --args='cc="clang" cxx="clang++" is_official_build=true skia_use_libjpeg_turbo=false' &&\
	ninja -C out/skia/ &&\
	cp ./out/skia/*.a ./../../build/

transcoding: clean ./build/libskia.a
	mkdir -p ./build && \
	cp ./src/opengl/shaders/* ./build/
	g++ -g -Wall -std=c++17 ./src/all.hpp -o ./build/all.hpp.gch \
	&& g++ -g -std=c++17 -Wall -o build/transcoding \
	-lboost_date_time -lavformat -lavcodec -lswscale -lz -lglfw -lavutil -framework OpenGL \
	./src/*.cpp ./src/opengl/*.cpp ./src/transcoding/*.cpp \
	 -I./src/ -I./src/opengl/ -I./src/transcoding/ -I ./build -include all.hpp -stdlib=libc++ -DGL_SILENCE_DEPRECATION=1 -DDEBUG=1 &&\
	 cd ./build &&\
	 ./transcoding ./../movies/small_bunny_1080p_60fps.mp4 ./../movies/dog.mp4  ./../movies/bunny_2s_gop.mp4 0.5 2 4

clean:
	rm -rf ./build/*
