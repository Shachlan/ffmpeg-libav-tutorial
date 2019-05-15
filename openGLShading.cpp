#include "openGLShading.h"

#include <string>

#include <simd/simd.h>
#include <tbb/tbb.h>

using namespace tbb;
 

  constexpr std::size_t simd_size = 4;

class InvertRendering {
    const uint8_t * _texture1;
    uint8_t * _output_buffer;
public:
    void operator()( const blocked_range<size_t>& r ) const {
        const uint8_t * texture1 = _texture1;
        uint8_t * output_buffer = _output_buffer;
        for( size_t j=r.begin(); j!=r.end(); ++j )  {
          auto i = j * simd_size;
          auto rgb = simd_make_uint4(texture1[i], texture1[i + 1], texture1[i + 2], texture1[i + 3]);
          auto res = 255 - rgb;
          output_buffer[i] = res.x;
          output_buffer[i + 1] = res.y;
          output_buffer[i + 2] = res.z;
          output_buffer[i + 3] = res.w;
        }
    }
    InvertRendering(const uint8_t * texture1, uint8_t * output_buffer) :
        _texture1(texture1), _output_buffer(output_buffer)
    {}
};

class BlendRendering {
    const uint8_t * _texture1;
    const uint8_t * _texture2;
    uint8_t * _output_buffer;
public:
    void operator()( const blocked_range<size_t>& r ) const {
        const uint8_t * texture1 = _texture1;
        const uint8_t * texture2 = _texture2;
        uint8_t * output_buffer = _output_buffer;
        for( size_t j=r.begin(); j!=r.end(); ++j )  {
          auto i = j * simd_size;
          auto rgb = simd_make_uint4(texture1[i], texture1[i + 1], texture1[i + 2], texture1[i + 3]);
          auto rgb2 = simd_make_uint4(texture2[i], texture2[i + 1], texture2[i + 2], texture2[i + 3]);
          auto res = (rgb2 + rgb) / 2;
          output_buffer[i] = res.x;
          output_buffer[i + 1] = res.y;
          output_buffer[i + 2] = res.z;
          output_buffer[i + 3] = res.w;
        }
    }
    BlendRendering(const uint8_t * texture1, const uint8_t * texture2, uint8_t * output_buffer) :
        _texture1(texture1), _texture2(texture2), _output_buffer(output_buffer)
    {}
};

struct TextureInfo {
  const uint8_t *buffer;
  int width; 
  int height;
};

int target_width;
int target_height;
float target_blend_ratio;

TextureInfo textures[2];
int latest_created_texture = -1;

uint32_t createTexture(void) {
  return ++latest_created_texture;
}

bool next_invert;

void setupOpenGL(int width, int height, float blend_ratio, char *canvasName) {
  target_width = width;
  target_height = height;
  target_blend_ratio = blend_ratio;
}

void loadTexture(uint32_t textureID, int width, int height, const uint8_t *buffer) {
  textures[textureID] = (TextureInfo) {
    .width = width,
    .height = height,
    .buffer = buffer
  };
}

void invertFrame(uint32_t textureID) {
  next_invert = true;
}

void blendFrames(uint32_t texture1ID, uint32_t texture2ID) {
  next_invert = false;
}

void getCurrentResults(int width, int height, uint8_t *outputBuffer) {
  auto size = width * height * 3;
  std::size_t vec_size = size / simd_size;
  auto texture1 = textures[0].buffer;
  auto texture2 = textures[1].buffer;
  if (next_invert) {
    parallel_for(blocked_range<size_t>(0, vec_size), InvertRendering(texture1, outputBuffer));
  }
  else {
    parallel_for(blocked_range<size_t>(0, vec_size), BlendRendering( texture1,texture2, outputBuffer));
  }
}

void tearDownOpenGL() {
}
