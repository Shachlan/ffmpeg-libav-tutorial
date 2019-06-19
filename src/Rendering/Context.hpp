
namespace WRERendering {
class Context {
public:
  virtual ~Context();
  void render_according_to_model();
  void load_model(RenderingModel model);
  void render_with_timestamp(double timestamp);
  uint32_t get_texture();
  void release_texture(uint32_t textureID);

protected:
  setup(int width, int height);

private:
  RenderingModel rendering_model;
};

#if FRONTEND == 1
class WebGLContext : public Context {
public:
  WebGLContext(int width, int height, char *canvasName);
  void load_model(string &&model);
  void render_with_timestamp(double timestamp);
};
#else

class OpenGLContext : public Context {
public:
  OpenGLContext(int width, int height);
  void get_current_frame(int width, int height, uint8_t *outputBuffer);
  void loadTexture(uint32_t texture_name, int width, int height, const uint8_t *buffer);
};

#endif
}  // namespace WRERendering
