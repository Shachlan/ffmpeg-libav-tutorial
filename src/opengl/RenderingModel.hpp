#include <vector>

using std::string_view;

class Layer {
public:
  constexpr Layer(string_view type) : type(type) {
  }

  constexpr string_view get_type() const noexcept {
    return type;
  }

private:
  string_view type;
};
using Layers = std::vector<shared_ptr<Layer>>;

class VideoLayer : public Layer {
public:
  static constexpr string_view Type = "video";

  constexpr VideoLayer(string_view file_name, double expected_framerate, double start_time,
                       double duration, double speed_ratio)
      : Layer(Type),
        file_name(file_name),
        expected_framerate(expected_framerate),
        start_time(start_time),
        duration(duration),
        speed_ratio(speed_ratio) {
  }

  constexpr string_view get_file_name() const noexcept {
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
  string_view file_name;
  double expected_framerate;
  double start_time;
  double duration;
  double speed_ratio;
};

class TextureLayer : public Layer {
public:
  static constexpr string_view Type = "texture";

  constexpr TextureLayer(uint32_t source_texture) : Layer(Type), source_texture(source_texture) {
  }

  constexpr uint32_t get_source_texture() const noexcept {
    return source_texture;
  }

private:
  uint32_t source_texture;
};

class TextLayer : public Layer {
public:
  static constexpr string_view Type = "text";

  constexpr TextLayer(string_view text, string_view font, int font_size)
      : Layer(Type), text(text), font(font), font_size(font_size) {
  }

  constexpr string_view get_text() const noexcept {
    return text;
  }

  constexpr string_view get_font() const noexcept {
    return font;
  }

  constexpr int get_font_size() const noexcept {
    return font_size;
  }

private:
  string_view text;
  string_view font;
  int font_size;
};

class RenderingModel {
public:
  static unique_ptr<Layers> get_layers(string model);
};
