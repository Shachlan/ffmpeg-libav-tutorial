
namespace WRESkiaRendering {
class SkiaException : std::exception {
public:
  SkiaException(string message) {
    full_description = message;
  }

  /// Error message of the exception.
  const char *what() const noexcept {
    return full_description.c_str();
  }

  /// Description of the error that caused the exception.
  string full_description;
};
}  // namespace WRESkiaRendering
