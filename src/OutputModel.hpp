#include <nlohmann/json.hpp>
using json = nlohmann::json;

using std::string;

class OutputModel {
public:
  OutputModel(json model);
  int width;
  int height;
  string source;
};
