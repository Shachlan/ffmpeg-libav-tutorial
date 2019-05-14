VIDEO_URL := http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

ARGS=./movies/small_bunny_1080p_60fps.mp4 ./movies/dog.mp4  ./movies/bunny_1s_gop.mp4 0.5 100

transcoding: ./*.cpp ./Transcoding/*.cpp
	@g++ -g -w -std=c++17 all.hpp -o ./build/all.hpp.gch \
	&& g++ -g -std=c++17 -w -o build/transcoding-apple-simd -lboost_date_time -lavformat -lavcodec -lswscale -lz -lglfw -lavutil -framework OpenGL *.cpp ./Transcoding/*.cpp -I./ -I ./build -include all.hpp \

clean:
	@rm -rf ./build/*

run-all:
	@./build/transcoding-cpu ${ARGS} &&\
	@./build/transcoding-gpu ${ARGS} &&\
	@./build/transcoding-xsimd ${ARGS} &&\
	@./build/transcoding-apple-simd .${ARGS} &&\
	@./build/transcoding-cpu ${ARGS} &&\
	@./build/transcoding-gpu ${ARGS} &&\
	@./build/transcoding-xsimd ${ARGS} &&\
	@./build/transcoding-apple-simd .${ARGS} &&\
	@./build/transcoding-cpu ${ARGS}&&\
	@./build/transcoding-gpu ${ARGS} &&\
	@./build/transcoding-xsimd ${ARGS} &&\
	@./build/transcoding-apple-simd ${ARGS}

