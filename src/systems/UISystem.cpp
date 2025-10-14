#include "../../include/GameEngine/systems/UISystem.h"
#include "../../include/GameEngine/core/SystemImpl.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>

namespace FoundryEngine {

// UI Manager Implementation
class DefaultUIManagerImpl : public SystemImplBase<DefaultUIManagerImpl> {
private:
    std::unordered_map<std::string, std::unique_ptr<UIElement>> uiElements_;
    std::vector<UIElement*> rootElements_;
    UIElement* focusedElement_ = nullptr;
    UIElement* hoveredElement_ = nullptr;

    Vector2 screenSize_ = Vector2(1920, 1080);
    Vector2 mousePosition_ = Vector2(0, 0);
    bool mousePressed_ = false;

    int elementsRendered_ = 0;
    int totalElements_ = 0;

    friend class SystemImplBase<DefaultUIManagerImpl>;

    bool onInitialize() override {
        std::cout << "UI Manager initialized with screen size: " <<
                  screenSize_.x << "x" << screenSize_.y << std::endl;
        return true;
    }

    void onShutdown() override {
        uiElements_.clear();
        rootElements_.clear();
        focusedElement_ = nullptr;
        hoveredElement_ = nullptr;
        std::cout << "UI Manager shutdown" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Update animations and input handling
        for (auto& pair : uiElements_) {
            if (pair.second) {
                pair.second->update(deltaTime);
            }
        }

        // Handle input
        updateInput();
    }

    void updateInput() {
        // Update hover state
        hoveredElement_ = nullptr;
        for (auto element : rootElements_) {
            if (element && element->isVisible() && element->containsPoint(mousePosition_)) {
                hoveredElement_ = element;
                break;
            }
        }

        // Handle mouse events
        if (mousePressed_ && hoveredElement_) {
            if (focusedElement_ != hoveredElement_) {
                if (focusedElement_) {
                    focusedElement_->onFocusLost();
                }
                focusedElement_ = hoveredElement_;
                focusedElement_->onFocusGained();
            }
            hoveredElement_->onMouseDown(mousePosition_);
        }
    }

public:
    DefaultUIManagerImpl() : SystemImplBase("DefaultUIManager") {}

    std::string getStatistics() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "UI Stats - Elements: %d total, %d rendered, Focused: %s",
                 totalElements_, elementsRendered_,
                 focusedElement_ ? focusedElement_->getName().c_str() : "none");
        return std::string(buffer);
    }

    void render() {
        if (!isInitialized()) return;

        elementsRendered_ = 0;

        // Render all root elements
        for (auto element : rootElements_) {
            if (element && element->isVisible()) {
                renderElement(element);
            }
        }
    }

    void renderElement(UIElement* element) {
        if (!element) return;

        // Render the element
        element->render();

        // Render children
        for (auto child : element->getChildren()) {
            if (child && child->isVisible()) {
                renderElement(child);
            }
        }

        elementsRendered_++;
    }

    UIElement* createElement(const std::string& name, UIElementType type) {
        std::unique_ptr<UIElement> element;

        switch (type) {
            case UIElementType::Panel:
                element = std::make_unique<UIPanel>();
                break;
            case UIElementType::Button:
                element = std::make_unique<UIButton>();
                break;
            case UIElementType::Label:
                element = std::make_unique<UILabel>();
                break;
            case UIElementType::TextBox:
                element = std::make_unique<UITextBox>();
                break;
            case UIElementType::Image:
                element = std::make_unique<UIImage>();
                break;
            default:
                return nullptr;
        }

        element->setName(name);
        UIElement* elementPtr = element.get();
        uiElements_[name] = std::move(element);
        totalElements_++;

        return elementPtr;
    }

    void destroyElement(const std::string& name) {
        auto it = uiElements_.find(name);
        if (it != uiElements_.end()) {
            // Remove from root elements if present
            rootElements_.erase(
                std::remove(rootElements_.begin(), rootElements_.end(), it->second.get()),
                rootElements_.end());

            // Clear focus if this element was focused
            if (focusedElement_ == it->second.get()) {
                focusedElement_ = nullptr;
            }
            if (hoveredElement_ == it->second.get()) {
                hoveredElement_ = nullptr;
            }

            uiElements_.erase(it);
            totalElements_--;
        }
    }

    UIElement* getElement(const std::string& name) const {
        auto it = uiElements_.find(name);
        return (it != uiElements_.end()) ? it->second.get() : nullptr;
    }

    void addToRoot(UIElement* element) {
        if (element && std::find(rootElements_.begin(), rootElements_.end(), element) == rootElements_.end()) {
            rootElements_.push_back(element);
        }
    }

    void removeFromRoot(UIElement* element) {
        rootElements_.erase(
            std::remove(rootElements_.begin(), rootElements_.end(), element),
            rootElements_.end());
    }

    std::vector<UIElement*> getRootElements() const {
        return rootElements_;
    }

    void setScreenSize(const Vector2& size) {
        screenSize_ = size;
    }

    Vector2 getScreenSize() const {
        return screenSize_;
    }

    void setMousePosition(const Vector2& position) {
        mousePosition_ = position;
    }

    Vector2 getMousePosition() const {
        return mousePosition_;
    }

    void setMousePressed(bool pressed) {
        mousePressed_ = pressed;
    }

    bool isMousePressed() const {
        return mousePressed_;
    }

    UIElement* getFocusedElement() const {
        return focusedElement_;
    }

    UIElement* getHoveredElement() const {
        return hoveredElement_;
    }

    void setFocus(UIElement* element) {
        if (focusedElement_ != element) {
            if (focusedElement_) {
                focusedElement_->onFocusLost();
            }
            focusedElement_ = element;
            if (focusedElement_) {
                focusedElement_->onFocusGained();
            }
        }
    }

    void clearFocus() {
        if (focusedElement_) {
            focusedElement_->onFocusLost();
            focusedElement_ = nullptr;
        }
    }

    std::vector<std::string> getElementNames() const {
        std::vector<std::string> names;
        for (const auto& pair : uiElements_) {
            names.push_back(pair.first);
        }
        return names;
    }
};

DefaultUIManager::DefaultUIManager() : impl_(std::make_unique<DefaultUIManagerImpl>()) {}
DefaultUIManager::~DefaultUIManager() = default;

bool DefaultUIManager::initialize() { return impl_->initialize(); }
void DefaultUIManager::shutdown() { impl_->shutdown(); }
void DefaultUIManager::update(float deltaTime) { impl_->update(deltaTime); }
void DefaultUIManager::render() { impl_->render(); }

UIElement* DefaultUIManager::createElement(const std::string& name, UIElementType type) { return impl_->createElement(name, type); }
void DefaultUIManager::destroyElement(const std::string& name) { impl_->destroyElement(name); }
UIElement* DefaultUIManager::getElement(const std::string& name) const { return impl_->getElement(name); }
void DefaultUIManager::addToRoot(UIElement* element) { impl_->addToRoot(element); }
void DefaultUIManager::removeFromRoot(UIElement* element) { impl_->removeFromRoot(element); }
std::vector<UIElement*> DefaultUIManager::getRootElements() const { return impl_->getRootElements(); }
void DefaultUIManager::setScreenSize(const Vector2& size) { impl_->setScreenSize(size); }
Vector2 DefaultUIManager::getScreenSize() const { return impl_->getScreenSize(); }
void DefaultUIManager::setMousePosition(const Vector2& position) { impl_->setMousePosition(position); }
Vector2 DefaultUIManager::getMousePosition() const { return impl_->getMousePosition(); }
void DefaultUIManager::setMousePressed(bool pressed) { impl_->setMousePressed(pressed); }
bool DefaultUIManager::isMousePressed() const { return impl_->isMousePressed(); }
UIElement* DefaultUIManager::getFocusedElement() const { return impl_->getFocusedElement(); }
UIElement* DefaultUIManager::getHoveredElement() const { return impl_->getHoveredElement(); }
void DefaultUIManager::setFocus(UIElement* element) { impl_->setFocus(element); }
void DefaultUIManager::clearFocus() { impl_->clearFocus(); }
std::vector<std::string> DefaultUIManager::getElementNames() const { return impl_->getElementNames(); }

} // namespace FoundryEngine
