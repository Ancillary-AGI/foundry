#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include "../math/Vector2.h"
#include "../math/Vector3.h"

namespace FoundryEngine {

class Font;
class Texture;

enum class UIAnchor {
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

enum class UILayoutType {
    None,
    Horizontal,
    Vertical,
    Grid
};

struct UIRect {
    float x, y, width, height;
    UIRect(float x = 0, float y = 0, float w = 0, float h = 0) : x(x), y(y), width(w), height(h) {}
};

struct UIColor {
    float r, g, b, a;
    UIColor(float r = 1, float g = 1, float b = 1, float a = 1) : r(r), g(g), b(b), a(a) {}
};

class UIElement {
public:
    virtual ~UIElement() = default;
    
    virtual void setPosition(const Vector2& position) = 0;
    virtual Vector2 getPosition() const = 0;
    
    virtual void setSize(const Vector2& size) = 0;
    virtual Vector2 getSize() const = 0;
    
    virtual void setAnchor(UIAnchor anchor) = 0;
    virtual UIAnchor getAnchor() const = 0;
    
    virtual void setVisible(bool visible) = 0;
    virtual bool isVisible() const = 0;
    
    virtual void setEnabled(bool enabled) = 0;
    virtual bool isEnabled() const = 0;
    
    virtual void setParent(UIElement* parent) = 0;
    virtual UIElement* getParent() const = 0;
    
    virtual void addChild(UIElement* child) = 0;
    virtual void removeChild(UIElement* child) = 0;
    virtual std::vector<UIElement*> getChildren() const = 0;
    
    virtual UIRect getRect() const = 0;
    virtual UIRect getWorldRect() const = 0;
    
    virtual bool containsPoint(const Vector2& point) const = 0;
    
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
    
    virtual void onMouseEnter() {}
    virtual void onMouseExit() {}
    virtual void onMouseDown(int button) {}
    virtual void onMouseUp(int button) {}
    virtual void onClick() {}
    virtual void onDoubleClick() {}
    virtual void onDrag(const Vector2& delta) {}
    virtual void onKeyDown(int key) {}
    virtual void onKeyUp(int key) {}
    virtual void onTextInput(const std::string& text) {}
};

class UIPanel : public UIElement {
public:
    virtual void setBackgroundColor(const UIColor& color) = 0;
    virtual UIColor getBackgroundColor() const = 0;
    
    virtual void setBackgroundTexture(Texture* texture) = 0;
    virtual Texture* getBackgroundTexture() const = 0;
    
    virtual void setBorderWidth(float width) = 0;
    virtual float getBorderWidth() const = 0;
    
    virtual void setBorderColor(const UIColor& color) = 0;
    virtual UIColor getBorderColor() const = 0;
    
    virtual void setCornerRadius(float radius) = 0;
    virtual float getCornerRadius() const = 0;
};

class UIButton : public UIElement {
public:
    virtual void setText(const std::string& text) = 0;
    virtual std::string getText() const = 0;
    
    virtual void setFont(Font* font) = 0;
    virtual Font* getFont() const = 0;
    
    virtual void setTextColor(const UIColor& color) = 0;
    virtual UIColor getTextColor() const = 0;
    
    virtual void setBackgroundColor(const UIColor& color) = 0;
    virtual UIColor getBackgroundColor() const = 0;
    
    virtual void setHoverColor(const UIColor& color) = 0;
    virtual UIColor getHoverColor() const = 0;
    
    virtual void setPressedColor(const UIColor& color) = 0;
    virtual UIColor getPressedColor() const = 0;
    
    virtual void setDisabledColor(const UIColor& color) = 0;
    virtual UIColor getDisabledColor() const = 0;
    
    virtual void setOnClickCallback(std::function<void()> callback) = 0;
};

class UILabel : public UIElement {
public:
    virtual void setText(const std::string& text) = 0;
    virtual std::string getText() const = 0;
    
    virtual void setFont(Font* font) = 0;
    virtual Font* getFont() const = 0;
    
    virtual void setTextColor(const UIColor& color) = 0;
    virtual UIColor getTextColor() const = 0;
    
    virtual void setFontSize(float size) = 0;
    virtual float getFontSize() const = 0;
    
    virtual void setTextAlignment(int alignment) = 0;
    virtual int getTextAlignment() const = 0;
    
    virtual void setWordWrap(bool wrap) = 0;
    virtual bool getWordWrap() const = 0;
};

class UIImage : public UIElement {
public:
    virtual void setTexture(Texture* texture) = 0;
    virtual Texture* getTexture() const = 0;
    
    virtual void setColor(const UIColor& color) = 0;
    virtual UIColor getColor() const = 0;
    
    virtual void setUVRect(const UIRect& uvRect) = 0;
    virtual UIRect getUVRect() const = 0;
    
    virtual void setPreserveAspect(bool preserve) = 0;
    virtual bool getPreserveAspect() const = 0;
};

class UITextInput : public UIElement {
public:
    virtual void setText(const std::string& text) = 0;
    virtual std::string getText() const = 0;
    
    virtual void setPlaceholder(const std::string& placeholder) = 0;
    virtual std::string getPlaceholder() const = 0;
    
    virtual void setFont(Font* font) = 0;
    virtual Font* getFont() const = 0;
    
    virtual void setTextColor(const UIColor& color) = 0;
    virtual UIColor getTextColor() const = 0;
    
    virtual void setBackgroundColor(const UIColor& color) = 0;
    virtual UIColor getBackgroundColor() const = 0;
    
    virtual void setMaxLength(int maxLength) = 0;
    virtual int getMaxLength() const = 0;
    
    virtual void setPasswordMode(bool password) = 0;
    virtual bool isPasswordMode() const = 0;
    
    virtual void setOnTextChangedCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void setOnSubmitCallback(std::function<void(const std::string&)> callback) = 0;
};

class UISlider : public UIElement {
public:
    virtual void setValue(float value) = 0;
    virtual float getValue() const = 0;
    
    virtual void setMinValue(float minValue) = 0;
    virtual float getMinValue() const = 0;
    
    virtual void setMaxValue(float maxValue) = 0;
    virtual float getMaxValue() const = 0;
    
    virtual void setStep(float step) = 0;
    virtual float getStep() const = 0;
    
    virtual void setOrientation(bool horizontal) = 0;
    virtual bool isHorizontal() const = 0;
    
    virtual void setHandleColor(const UIColor& color) = 0;
    virtual UIColor getHandleColor() const = 0;
    
    virtual void setTrackColor(const UIColor& color) = 0;
    virtual UIColor getTrackColor() const = 0;
    
    virtual void setOnValueChangedCallback(std::function<void(float)> callback) = 0;
};

class UIScrollView : public UIElement {
public:
    virtual void setContentSize(const Vector2& size) = 0;
    virtual Vector2 getContentSize() const = 0;
    
    virtual void setScrollPosition(const Vector2& position) = 0;
    virtual Vector2 getScrollPosition() const = 0;
    
    virtual void setHorizontalScrollEnabled(bool enabled) = 0;
    virtual bool isHorizontalScrollEnabled() const = 0;
    
    virtual void setVerticalScrollEnabled(bool enabled) = 0;
    virtual bool isVerticalScrollEnabled() const = 0;
    
    virtual void setScrollSensitivity(float sensitivity) = 0;
    virtual float getScrollSensitivity() const = 0;
    
    virtual void scrollTo(const Vector2& position, bool animated = false) = 0;
    virtual void scrollBy(const Vector2& delta, bool animated = false) = 0;
};

class UILayout : public UIElement {
public:
    virtual void setLayoutType(UILayoutType type) = 0;
    virtual UILayoutType getLayoutType() const = 0;
    
    virtual void setPadding(float padding) = 0;
    virtual float getPadding() const = 0;
    
    virtual void setSpacing(float spacing) = 0;
    virtual float getSpacing() const = 0;
    
    virtual void setChildAlignment(int alignment) = 0;
    virtual int getChildAlignment() const = 0;
    
    virtual void updateLayout() = 0;
    virtual void setAutoLayout(bool auto_layout) = 0;
    virtual bool isAutoLayout() const = 0;
};

class UICanvas {
public:
    virtual ~UICanvas() = default;
    
    virtual void setSize(const Vector2& size) = 0;
    virtual Vector2 getSize() const = 0;
    
    virtual void setScale(float scale) = 0;
    virtual float getScale() const = 0;
    
    virtual void addElement(UIElement* element) = 0;
    virtual void removeElement(UIElement* element) = 0;
    virtual std::vector<UIElement*> getElements() const = 0;
    
    virtual UIElement* getElementAt(const Vector2& position) = 0;
    virtual std::vector<UIElement*> getElementsAt(const Vector2& position) = 0;
    
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
    
    virtual void handleMouseMove(const Vector2& position) = 0;
    virtual void handleMouseDown(int button, const Vector2& position) = 0;
    virtual void handleMouseUp(int button, const Vector2& position) = 0;
    virtual void handleMouseWheel(float delta, const Vector2& position) = 0;
    virtual void handleKeyDown(int key) = 0;
    virtual void handleKeyUp(int key) = 0;
    virtual void handleTextInput(const std::string& text) = 0;
};

class UIManager {
public:
    virtual ~UIManager() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
    
    virtual UICanvas* createCanvas() = 0;
    virtual void destroyCanvas(UICanvas* canvas) = 0;
    virtual std::vector<UICanvas*> getCanvases() const = 0;
    
    virtual UIPanel* createPanel() = 0;
    virtual UIButton* createButton() = 0;
    virtual UILabel* createLabel() = 0;
    virtual UIImage* createImage() = 0;
    virtual UITextInput* createTextInput() = 0;
    virtual UISlider* createSlider() = 0;
    virtual UIScrollView* createScrollView() = 0;
    virtual UILayout* createLayout() = 0;
    
    virtual void destroyElement(UIElement* element) = 0;
    
    virtual Font* loadFont(const std::string& path, float size) = 0;
    virtual void unloadFont(Font* font) = 0;
    
    virtual void setDefaultFont(Font* font) = 0;
    virtual Font* getDefaultFont() const = 0;
    
    virtual void setTheme(const std::string& themePath) = 0;
    virtual std::string getCurrentTheme() const = 0;
    
    virtual void enableDebugDraw(bool enable) = 0;
    virtual bool isDebugDrawEnabled() const = 0;
};

} // namespace FoundryEngine