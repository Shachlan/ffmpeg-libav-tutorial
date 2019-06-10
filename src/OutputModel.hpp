#include <nlohmann/json.hpp>
using json = nlohmann::json;

using std::string;

class OutputModel {
public:
  explicit OutputModel(json &&model);
  int width;
  int height;
  string source;
};
