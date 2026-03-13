#pragma once

#include "common.hpp"
#include "../../render/module.hpp"

RAMPAGE_GUI_START

static constexpr glm::vec4 WHITE       = glm::vec4(1, 1, 1, 1);
static constexpr glm::vec4 BLACK       = glm::vec4(0, 0, 0, 1);
static constexpr glm::vec4 RED         = glm::vec4(1, 0, 0, 1);
static constexpr glm::vec4 GREEN       = glm::vec4(0, 1, 0, 1);
static constexpr glm::vec4 BLUE        = glm::vec4(0, 0, 1, 1);
static constexpr glm::vec4 YELLOW      = glm::vec4(1, 1, 0, 1);
static constexpr glm::vec4 CYAN        = glm::vec4(0, 1, 1, 1);
static constexpr glm::vec4 MAGENTA     = glm::vec4(1, 0, 1, 1);
static constexpr glm::vec4 ORANGE      = glm::vec4(1, 0.5f, 0, 1);
static constexpr glm::vec4 PURPLE      = glm::vec4(0.5f, 0, 0.5f, 1);
static constexpr glm::vec4 PINK        = glm::vec4(1, 0.75f, 0.8f, 1);
static constexpr glm::vec4 BROWN       = glm::vec4(0.6f, 0.3f, 0.1f, 1);
static constexpr glm::vec4 LIME        = glm::vec4(0.5f, 1, 0, 1);
static constexpr glm::vec4 TEAL        = glm::vec4(0, 0.5f, 0.5f, 1);
static constexpr glm::vec4 NAVY        = glm::vec4(0, 0, 0.5f, 1);
static constexpr glm::vec4 MAROON      = glm::vec4(0.5f, 0, 0, 1);
static constexpr glm::vec4 OLIVE       = glm::vec4(0.5f, 0.5f, 0, 1);
static constexpr glm::vec4 SILVER      = glm::vec4(0.75f, 0.75f, 0.75f, 1);
static constexpr glm::vec4 GRAY        = glm::vec4(0.5f, 0.5f, 0.5f, 1);
static constexpr glm::vec4 DARK_GRAY   = glm::vec4(0.25f, 0.25f, 0.25f, 1);
static constexpr glm::vec4 LIGHT_GRAY  = glm::vec4(0.85f, 0.85f, 0.85f, 1);
static constexpr glm::vec4 GOLD        = glm::vec4(1.0f, 0.84f, 0.0f, 1);
static constexpr glm::vec4 SKY_BLUE    = glm::vec4(0.53f, 0.81f, 0.92f, 1);
static constexpr glm::vec4 CORAL       = glm::vec4(1.0f, 0.5f, 0.31f, 1);
static constexpr glm::vec4 INDIGO      = glm::vec4(0.29f, 0.0f, 0.51f, 1);

enum class AttributeType {
  Relative,
  Absolute
};

template<typename ScalarType>
class Attribute {
public:
  Attribute() = default;
  Attribute(const Attribute<ScalarType>& other) = default;
  Attribute(AttributeType type, const ScalarType& value) : type(type), value(value) {}

  Attribute& operator=(const Attribute& other) = default;
  Attribute& operator=(const ScalarType& newValue) {
    type = AttributeType::Absolute;
    value = newValue;
    return *this;
  }

  void setType(AttributeType newType) {
    type = newType;
  }

  void setValue(const ScalarType& newValue) {
    value = newValue;
  }

  AttributeType getType() const {
    return type;
  }

  const ScalarType& getValue() const {
    return value;
  }

  ScalarType computeValue(const ScalarType& parentSize) const {
    if (type == AttributeType::Relative) {
      return parentSize * value;
    } else {
      return value;
    }
  }

  ScalarType* operator->() {
    return &value;
  }

  ScalarType& operator*() {
    return value;
  }

private:
  AttributeType type = AttributeType::Absolute;
  ScalarType value = ScalarType{0};
};

class Element;

class Layout {
public:
  using Ptr = std::shared_ptr<Layout>;
  virtual ~Layout() = default;

  virtual void apply(Element& element) = 0;

protected:
  static void setComputedPos(Element& element, const glm::vec2& pos);
  static void setComputedSize(Element& element, const glm::vec2& size);
  static glm::vec2 getMeasuredPos(const Element& element);
  static glm::vec2 getMeasuredSize(const Element& element);
  static glm::vec2 getMeasuredMargin(const Element& element);
  static glm::vec2 getMeasuredPadding(const Element& element);
  static glm::vec2 getMeasuredBorder(const Element& element);
};

using EventCallback = std::function<void()>;

class Element : public std::enable_shared_from_this<Element> {
  friend class Layout;
public:
  using Ptr = std::shared_ptr<Element>;

  static Ptr create() {
    return std::make_shared<Element>();
  }

  Element::Ptr getParent() const {
    return m_parent;
  }

  void add(const Element::Ptr& element) {
    m_elements.push_back(element);
    element->m_parent = shared_from_this();
  }

  void remove(const Element::Ptr& element) {
    m_elements.erase(std::remove(m_elements.begin(), m_elements.end(), element), m_elements.end());
    element->m_parent.reset();  
  }

  void removeAll() {
    for (const auto& element : m_elements) {
      element->m_parent.reset();
    }
    m_elements.clear();
  }

  void setLayout(const Layout::Ptr& layout) {
    m_layout = layout;
  }

  void measure(const glm::vec2& parentPos, const glm::vec2& parentSize) {
    m_measuredMargin = margin.computeValue(parentSize);
    m_measuredPadding = padding.computeValue(parentSize);
    m_measuredBorder = border.computeValue(parentSize);
    m_measuredSize = size.computeValue(parentSize) + m_measuredBorder * 2.0f + m_measuredPadding * 2.0f;
    m_measuredPos = parentPos + pos.computeValue(parentSize) + m_measuredMargin;

    for (const auto& element : m_elements) {
      element->measure(m_measuredPos, m_measuredSize);
    }
  }

  void layout(const glm::vec2& parentPos, const glm::vec2& parentSize) {
    if(m_layout) {
      m_layout->apply(*this);
    }

    for (const auto& element : m_elements) {
      element->layout(m_measuredPos, m_measuredSize);
    }
  }

  bool contains(const glm::vec2& point) const {
    return point.x >= m_computedPos.x && point.x <= m_computedPos.x + m_computedSize.x &&
           point.y >= m_computedPos.y && point.y <= m_computedPos.y + m_computedSize.y;
  }

public: /* PUBLIC MEMBERS. */
  bool isVisible = true;
  bool isEnabled = true;
  Attribute<glm::vec2> pos;
  Attribute<glm::vec2> size;
  Attribute<glm::vec2> margin;
  Attribute<glm::vec2> padding;
  Attribute<glm::vec2> border;

  glm::vec4 backgroundColor = glm::vec4(1, 1, 1, 1);
  glm::vec4 borderColor = glm::vec4(0, 0, 0, 1);

  EventCallback onMousePress;
  EventCallback onDoubleClick;
  EventCallback onMouseRelease;
  EventCallback onMouseEnter;
  EventCallback onMouseLeave;
protected:
  Layout::Ptr m_layout;
  std::vector<Element::Ptr> m_elements;
  Element::Ptr m_parent;

  // ALL TOP LEFT RELATIVE, COMPUTE CENTER POS + SIZE / 2 FOR RENDERING
  glm::vec2 m_computedPos = glm::vec2{0};
  glm::vec2 m_computedSize = glm::vec2{0};
  glm::vec2 m_measuredPos = glm::vec2{0};
  glm::vec2 m_measuredSize = glm::vec2{0};
  glm::vec2 m_measuredMargin = glm::vec2{0};
  glm::vec2 m_measuredPadding = glm::vec2{0};
  glm::vec2 m_measuredBorder = glm::vec2{0};
};

inline void Layout::setComputedPos(Element& element, const glm::vec2& pos)  {
  element.m_computedPos = pos;
}

inline void Layout::setComputedSize(Element& element, const glm::vec2& size) {
  element.m_computedSize = size;
}

inline glm::vec2 Layout::getMeasuredPos(const Element& element) {
  return element.m_measuredPos;
}

inline glm::vec2 Layout::getMeasuredSize(const Element& element) {
  return element.m_measuredSize;
}

inline glm::vec2 Layout::getMeasuredMargin(const Element& element) {
  return element.m_measuredMargin;
}

inline glm::vec2 Layout::getMeasuredPadding(const Element& element) {
  return element.m_measuredPadding;
}

inline glm::vec2 Layout::getMeasuredBorder(const Element& element) {
  return element.m_measuredBorder;
}

/**
 * Elements are placed as is, with no care for other elements. This is the most basic layout, and can be used for simple cases or as a base for more complex layouts.
 */
class AbsoluteLayout : public Layout {
public:
  using Ptr = std::shared_ptr<AbsoluteLayout>;

  void apply(Element& element) override {
    setComputedPos(element, getMeasuredPos(element));
    setComputedSize(element, getMeasuredSize(element));
  }

protected:
};

class Panel : public Element {
public:  
using Ptr = std::shared_ptr<Panel>;

  static Ptr create() {
    return std::make_shared<Panel>();
  }
};

class Picture : public Element {
public:
  using Ptr = std::shared_ptr<Picture>;

  static Ptr create() {
    return std::make_shared<Picture>();
  }

  TextureId texture = TextureId::null();
};

class Label : public Element {
public:
  using Ptr = std::shared_ptr<Label>;

  static Ptr create() {
    return std::make_shared<Label>();
  }

  std::string text;
  float textSize = 14.0f; // in pixels
  glm::vec4 textColor = glm::vec4(1, 1, 1, 1);
};

enum CloseBehaviour {
  Hide,
  Destroy
};

class ChildWindow : public Element {
public:
  using Ptr = std::shared_ptr<ChildWindow>;

  static Ptr create() {
    return std::make_shared<ChildWindow>();
  }

  EventCallback onClose;
  EventCallback onPositionChange;

  std::string title;
  Attribute<float> titleBarHeight;
  glm::vec4 titleColor = glm::vec4(1, 1, 1, 1);
  glm::vec4 titleBarColor = glm::vec4(0.2f, 0.2f, 0.2f, 1);
  bool isDraggable = true;
  bool isResizable = true;
  CloseBehaviour closeBehaviour = CloseBehaviour::Hide;
};

class Gui : public Element {
public:
  using Ptr = std::shared_ptr<Gui>;

  Gui(Render2D& render)
    : m_render(render) {}

  static Ptr create(Render2D& render) {
    return std::make_shared<Gui>(render);
  }

  void poll(SDL_Event& event) {

  }

  void update() {
    glm::vec2 windowSize = m_render.getWindowSize();
    measure(glm::vec2{0}, windowSize);
    layout(glm::vec2{0}, windowSize);
  }

  void render() {
    // submit draw commands
  }

private:
  Render2D& m_render;
};

RAMPAGE_GUI_END