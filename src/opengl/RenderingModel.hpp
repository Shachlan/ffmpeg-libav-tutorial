#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using std::string;

class Layer {
public:
  enum class Type { Video, Text, Texture };
  constexpr Layer(Type type) : type(type) {
  }

  constexpr Type get_type() const noexcept {
    return type;
  }

private:
  Type type;
};
using Layers = std::vector<shared_ptr<Layer>>;

class VideoLayer : public Layer {
public:
  VideoLayer(string file_name, double expected_framerate, double start_time, double duration,
             double speed_ratio)
      : Layer(Layer::Type::Video),
        file_name(file_name),
        expected_framerate(expected_framerate),
        start_time(start_time),
        duration(duration),
        speed_ratio(speed_ratio) {
  }

  string get_file_name() const noexcept {
    return file_name;
  }

  constexpr double get_expected_framerate() const noexcept {
    return expected_framerate;
  }

  constexpr double get_start_time() const noexcept {
    return start_time;
  }

  constexpr double get_duration() const noexcept {
    return duration;
  }

  constexpr double get_speed_ratio() const noexcept {
    return speed_ratio;
  }

private:
  string file_name;
  double expected_framerate;
  double start_time;
  double duration;
  double speed_ratio;
};

class TextureLayer : public Layer {
public:
  constexpr TextureLayer(uint32_t source_texture)
      : Layer(Layer::Type::Texture), source_texture(source_texture) {
  }

  constexpr uint32_t get_source_texture() const noexcept {
    return source_texture;
  }

private:
  uint32_t source_texture;
};

class TextLayer : public Layer {
public:
  TextLayer(string text, string font, int font_size)
      : Layer(Layer::Type::Text), text(text), font(font), font_size(font_size) {
  }

  string get_text() const noexcept {
    return text;
  }

  string get_font() const noexcept {
    return font;
  }

  constexpr int get_font_size() const noexcept {
    return font_size;
  }

private:
  string text;
  string font;
  int font_size;
};

class RenderingModel {
public:
  RenderingModel(json model);
  unique_ptr<Layers> layers;
};
