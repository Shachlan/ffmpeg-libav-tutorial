#include "OutputModel.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

OutputModel::OutputModel(json &&object) {
  width = object["width"].get<int>();
  height = object["height"].get<int>();
  source = object["target_file"].get<string>();
}
