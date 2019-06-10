#include "RenderingModel.hpp"

using layer_ptr = shared_ptr<Layer>;

static layer_ptr get_layer(json object) {
  auto type = object["type"];
  if (type == "video") {
    auto source = object["source"].get<string>();
    auto fps = object["expected_framerate"].get<double>();
    auto start = object["start_from"].get<double>();
    auto duration = object["duration"].get<double>();
    auto speed = object["speed_ratio"].get<double>();
    return std::make_shared<VideoLayer>(source, fps, start, duration, speed);
  }
  if (type == "texture") {
    return std::make_shared<TextureLayer>(object["texture"].get<uint32_t>());
  }
  if (type == "text") {
    return std::make_shared<TextLayer>(object["text"].get<string>(), object["font"].get<string>(),
                                       object["font_size"].get<int>());
  }
  throw std::runtime_error("unknown type: " + type);
}

static unique_ptr<Layers> get_layers(json layers_model) {
  auto vec = std::make_unique<Layers>();
  for (auto iter = layers_model.begin(); iter != layers_model.end(); iter++) {
    log_info("pushing");
    vec->push_back(get_layer(*iter));
  }
  return vec;
}

RenderingModel::RenderingModel(json &&object) {
  layers = get_layers(object);
}
