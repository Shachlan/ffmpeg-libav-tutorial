#include "RenderingModel.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using layer_ptr = shared_ptr<Layer>;

static layer_ptr get_layer(json object) {
  auto type = object["type"];
  if (type == "video") {
    return std::make_shared<VideoLayer>("", 0, 0, 0, 0);
  }
  if (type == "texture") {
  }
  if (type == "text") {
  }
  throw std::runtime_error("unknown type: " + type);
}

unique_ptr<Layers> RenderingModel::get_layers(string model) {
  auto layers = json::parse(model);
  auto vec = std::make_unique<Layers>(layers.size());
  for (auto iter = layers.begin(); iter != layers.end(); iter++) {
  }
  return vec;
}
