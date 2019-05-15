VIDEO_URL := http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

ARGS=./movies/small_bunny_1080p_60fps.mp4 ./movies/dog.mp4  ./build2/bunny_1s_gop.mp4 0.5 1

transcoding: ./*.cpp ./Transcoding/*.cpp
	@g++ -g -w -std=c++17 all.hpp -o ./build2/all.hpp.gch \
	&& g++ -g -std=c++17 -w -o build2/transcoding-apple-simd -lboost_date_time -lavformat -lavcodec -lswscale -lz -lglfw -ltbb -lavutil -framework OpenGL *.cpp ./Transcoding/*.cpp -I./ -I ./build2 -include all.hpp &&\
	./build2/transcoding-apple-simd ${ARGS}

clean:
	@rm -rf ./build2/*

run-all:
	@./build2/transcoding-cpu ${ARGS} &&\
	@./build2/transcoding-gpu ${ARGS} &&\
	@./build2/transcoding-xsimd ${ARGS} &&\
	@./build2/transcoding-apple-simd ${ARGS} &&\
	@./build2/transcoding-cpu ${ARGS} &&\
	@./build2/transcoding-gpu ${ARGS} &&\
	@./build2/transcoding-xsimd ${ARGS} &&\
	@./build2/transcoding-apple-simd ${ARGS} &&\
	@./build2/transcoding-cpu ${ARGS}&&\
	@./build2/transcoding-gpu ${ARGS} &&\
	@./build2/transcoding-xsimd ${ARGS} &&\
	@./build2/transcoding-apple-simd ${ARGS}

