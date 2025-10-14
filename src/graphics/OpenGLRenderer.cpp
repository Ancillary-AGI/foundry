#include "../../include/GameEngine/graphics/Renderer.h"
#include "../../include/GameEngine/core/SystemImpl.h"
#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace FoundryEngine {

class OpenGLRendererImpl : public SystemImplBase<OpenGLRendererImpl> {
private:
    GLFWwindow* window_ = nullptr;
    RenderSettings settings_;
    int drawCalls_ = 0;
    int triangles_ = 0;
    int vertices_ = 0;

    friend class SystemImplBase<OpenGLRendererImpl>;

    bool onInitialize() override {
        std::cout << "Initializing OpenGL Renderer..." << std::endl;

        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }

        // Set OpenGL version
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Create window (this should be handled by platform layer)
        window_ = glfwCreateWindow(1280, 720, "Foundry Engine", nullptr, nullptr);
        if (!window_) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window_);

        // Initialize GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            return false;
        }

        // Set up OpenGL state
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // Set clear color
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        std::cout << "OpenGL Renderer initialized successfully" << std::endl;
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

        return true;
    }

    void onShutdown() override {
        std::cout << "Shutting down OpenGL Renderer..." << std::endl;

        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }

        glfwTerminate();
        std::cout << "OpenGL Renderer shutdown complete" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        if (window_) {
            glfwPollEvents();
        }
    }

public:
    OpenGLRendererImpl() : SystemImplBase("OpenGLRenderer") {}

    void beginFrame() {
        if (!isInitialized()) return;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        resetStats();
    }

    void endFrame() {
        if (!isInitialized()) return;
        // End frame operations
    }

    void present() {
        if (!isInitialized() || !window_) return;

        glfwSwapBuffers(window_);
    }

    void drawMesh(Mesh* mesh, Material* material, const Matrix4& transform) {
        if (!isInitialized() || !mesh || !material) return;

        // Bind material/shader
        // Set uniforms (transform, material properties, etc.)
        // Bind vertex buffers
        // Draw elements

        glDrawElements(GL_TRIANGLES, mesh->getIndexCount(), GL_UNSIGNED_INT, nullptr);

        drawCalls_++;
        triangles_ += mesh->getIndexCount() / 3;
        vertices_ += mesh->getVertexCount();
    }

    void drawInstanced(Mesh* mesh, Material* material, const std::vector<Matrix4>& transforms) {
        if (!isInitialized() || !mesh || !material || transforms.empty()) return;

        // Enable instancing
        // Create instance buffer with transforms
        // Draw instanced

        glDrawElementsInstanced(GL_TRIANGLES, mesh->getIndexCount(), GL_UNSIGNED_INT,
                               nullptr, static_cast<GLsizei>(transforms.size()));

        drawCalls_++;
        triangles_ += (mesh->getIndexCount() / 3) * transforms.size();
        vertices_ += mesh->getVertexCount() * transforms.size();
    }

    void drawSkybox(Texture* skybox) {
        if (!isInitialized()) return;

        glDepthFunc(GL_LEQUAL);
        // Draw skybox cube with cubemap texture
        glDepthFunc(GL_LESS);
    }

    void drawUI() {
        if (!isInitialized()) return;
        // Implement UI rendering with immediate mode or VBOs
    }

    void setCamera(Camera* camera) {
        if (!isInitialized() || !camera) return;
        // Set camera matrices in shader uniforms
    }

    void setViewport(int x, int y, int width, int height) {
        if (!isInitialized()) return;
        glViewport(x, y, width, height);
    }

    void setLights(const std::vector<Light*>& lights) {
        if (!isInitialized()) return;
        // Update light uniforms in shaders
    }

    void setEnvironmentMap(Texture* envMap) {
        if (!isInitialized()) return;
        // Set environment map for reflections
    }

    RenderTarget* createRenderTarget(int width, int height, bool hdr) {
        if (!isInitialized()) return nullptr;

        // Create FBO with color and depth attachments
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Create color texture
        GLuint colorTexture;
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);

        GLenum internalFormat = hdr ? GL_RGBA16F : GL_RGBA8;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

        // Create depth texture
        GLuint depthTexture;
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Failed to create framebuffer" << std::endl;
            return nullptr;
        }

        // Return RenderTarget wrapper (would need to implement this)
        return nullptr;
    }

    void setRenderTarget(RenderTarget* target) {
        if (!isInitialized()) return;

        if (target) {
            // Bind custom FBO
            // glBindFramebuffer(GL_FRAMEBUFFER, target->getFBO());
        } else {
            // Bind default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    void clearRenderTarget(const Vector3& color, float depth) {
        if (!isInitialized()) return;

        glClearColor(color.x, color.y, color.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void applyPostProcessing() {
        if (!isInitialized()) return;
        // Implement post-processing with shaders
    }

    void setExposure(float exposure) {
        if (!isInitialized()) return;
        // Update post-processing shader uniforms
    }

    void setGamma(float gamma) {
        if (!isInitialized()) return;
        // Update gamma correction in shaders
    }

    void setRenderSettings(const RenderSettings& settings) {
        settings_ = settings;

        // Apply render settings
        if (settings.enableDepthTest) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }

        if (settings.enableBlending) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glDisable(GL_BLEND);
        }

        // Apply anti-aliasing settings
        switch (settings.antiAliasing) {
            case AntiAliasing::MSAA_2X:
                glEnable(GL_MULTISAMPLE);
                break;
            case AntiAliasing::MSAA_4X:
                glEnable(GL_MULTISAMPLE);
                break;
            default:
                glDisable(GL_MULTISAMPLE);
                break;
        }
    }

    RenderSettings getRenderSettings() const { return settings_; }

    void drawDebugLine(const Vector3& start, const Vector3& end, const Vector3& color) {
        if (!isInitialized()) return;

        glDisable(GL_DEPTH_TEST);
        glLineWidth(2.0f);

        GLfloat vertices[] = {
            start.x, start.y, start.z,
            end.x, end.y, end.z
        };

        GLfloat colors[] = {
            color.x, color.y, color.z,
            color.x, color.y, color.z
        };

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, vertices);
        glColorPointer(3, GL_FLOAT, 0, colors);
        glDrawArrays(GL_LINES, 0, 2);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

        glEnable(GL_DEPTH_TEST);
    }

    void drawDebugSphere(const Vector3& center, float radius, const Vector3& color) {
        if (!isInitialized()) return;
        // Implement sphere drawing with gluSphere or custom geometry
    }

    void drawDebugBox(const Vector3& center, const Vector3& size, const Vector3& color) {
        if (!isInitialized()) return;

        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        // Draw wireframe box
        glColor3f(color.x, color.y, color.z);

        glBegin(GL_QUADS);
        // Front face
        glVertex3f(center.x - size.x/2, center.y - size.y/2, center.z + size.z/2);
        glVertex3f(center.x + size.x/2, center.y - size.y/2, center.z + size.z/2);
        glVertex3f(center.x + size.x/2, center.y + size.y/2, center.z + size.z/2);
        glVertex3f(center.x - size.x/2, center.y + size.y/2, center.z + size.z/2);
        // Back face
        glVertex3f(center.x - size.x/2, center.y - size.y/2, center.z - size.z/2);
        glVertex3f(center.x + size.x/2, center.y - size.y/2, center.z - size.z/2);
        glVertex3f(center.x + size.x/2, center.y + size.y/2, center.z - size.z/2);
        glVertex3f(center.x - size.x/2, center.y + size.y/2, center.z - size.z/2);
        glEnd();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
    }

    int getDrawCalls() const { return drawCalls_; }
    int getTriangles() const { return triangles_; }
    int getVertices() const { return vertices_; }
    void resetStats() { drawCalls_ = 0; triangles_ = 0; vertices_ = 0; }

    std::string getStatistics() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "OpenGL Stats - Draw Calls: %d, Triangles: %d, Vertices: %d",
                 drawCalls_, triangles_, vertices_);
        return std::string(buffer);
    }
};

OpenGLRenderer::OpenGLRenderer() : impl_(std::make_unique<OpenGLRendererImpl>()) {}
OpenGLRenderer::~OpenGLRenderer() = default;

bool OpenGLRenderer::initialize() { return impl_->initialize(); }
void OpenGLRenderer::shutdown() { impl_->shutdown(); }
void OpenGLRenderer::beginFrame() { static_cast<OpenGLRendererImpl*>(impl_.get())->beginFrame(); }
void OpenGLRenderer::endFrame() { static_cast<OpenGLRendererImpl*>(impl_.get())->endFrame(); }
void OpenGLRenderer::present() { static_cast<OpenGLRendererImpl*>(impl_.get())->present(); }
void OpenGLRenderer::drawMesh(Mesh* mesh, Material* material, const Matrix4& transform) { static_cast<OpenGLRendererImpl*>(impl_.get())->drawMesh(mesh, material, transform); }
void OpenGLRenderer::drawInstanced(Mesh* mesh, Material* material, const std::vector<Matrix4>& transforms) { static_cast<OpenGLRendererImpl*>(impl_.get())->drawInstanced(mesh, material, transforms); }
void OpenGLRenderer::drawSkybox(Texture* skybox) { static_cast<OpenGLRendererImpl*>(impl_.get())->drawSkybox(skybox); }
void OpenGLRenderer::drawUI() { static_cast<OpenGLRendererImpl*>(impl_.get())->drawUI(); }
void OpenGLRenderer::setCamera(Camera* camera) { static_cast<OpenGLRendererImpl*>(impl_.get())->setCamera(camera); }
void OpenGLRenderer::setViewport(int x, int y, int width, int height) { static_cast<OpenGLRendererImpl*>(impl_.get())->setViewport(x, y, width, height); }
void OpenGLRenderer::setLights(const std::vector<Light*>& lights) { static_cast<OpenGLRendererImpl*>(impl_.get())->setLights(lights); }
void OpenGLRenderer::setEnvironmentMap(Texture* envMap) { static_cast<OpenGLRendererImpl*>(impl_.get())->setEnvironmentMap(envMap); }
RenderTarget* OpenGLRenderer::createRenderTarget(int width, int height, bool hdr) { return static_cast<OpenGLRendererImpl*>(impl_.get())->createRenderTarget(width, height, hdr); }
void OpenGLRenderer::setRenderTarget(RenderTarget* target) { static_cast<OpenGLRendererImpl*>(impl_.get())->setRenderTarget(target); }
void OpenGLRenderer::clearRenderTarget(const Vector3& color, float depth) { static_cast<OpenGLRendererImpl*>(impl_.get())->clearRenderTarget(color, depth); }
void OpenGLRenderer::applyPostProcessing() { static_cast<OpenGLRendererImpl*>(impl_.get())->applyPostProcessing(); }
void OpenGLRenderer::setExposure(float exposure) { static_cast<OpenGLRendererImpl*>(impl_.get())->setExposure(exposure); }
void OpenGLRenderer::setGamma(float gamma) { static_cast<OpenGLRendererImpl*>(impl_.get())->setGamma(gamma); }
void OpenGLRenderer::setRenderSettings(const RenderSettings& settings) { static_cast<OpenGLRendererImpl*>(impl_.get())->setRenderSettings(settings); }
RenderSettings OpenGLRenderer::getRenderSettings() const { return static_cast<OpenGLRendererImpl*>(impl_.get())->getRenderSettings(); }
void OpenGLRenderer::drawDebugLine(const Vector3& start, const Vector3& end, const Vector3& color) { static_cast<OpenGLRendererImpl*>(impl_.get())->drawDebugLine(start, end, color); }
void OpenGLRenderer::drawDebugSphere(const Vector3& center, float radius, const Vector3& color) { static_cast<OpenGLRendererImpl*>(impl_.get())->drawDebugSphere(center, radius, color); }
void OpenGLRenderer::drawDebugBox(const Vector3& center, const Vector3& size, const Vector3& color) { static_cast<OpenGLRendererImpl*>(impl_.get())->drawDebugBox(center, size, color); }
int OpenGLRenderer::getDrawCalls() const { return static_cast<OpenGLRendererImpl*>(impl_.get())->getDrawCalls(); }
int OpenGLRenderer::getTriangles() const { return static_cast<OpenGLRendererImpl*>(impl_.get())->getTriangles(); }
int OpenGLRenderer::getVertices() const { return static_cast<OpenGLRendererImpl*>(impl_.get())->getVertices(); }
void OpenGLRenderer::resetStats() { static_cast<OpenGLRendererImpl*>(impl_.get())->resetStats(); }

} // namespace FoundryEngine
