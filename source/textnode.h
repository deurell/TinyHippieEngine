#pragma once

#include "camera.h"
#include "scenenode.h"
#include "shader.h"
#include <memory>
#include <string_view>

class TextNode : public DL::SceneNode {
public:
    explicit TextNode(std::string_view glslVersionString,
                        DL::SceneNode *parentNode = nullptr, std::string text = "text");
    
    ~TextNode() override = default;
    
    void init() override;
    void update(float delta) override;
    void render(float delta) override;
    void onScreenSizeChanged(glm::vec2 size) override;
    DL::Camera& getCamera() { return *camera_; }

private:
void initCamera();
void initComponents();

std::unique_ptr<DL::Camera> camera_;
glm::vec2 screenSize_{0, 0};
std::string glslVersionString_;
std::string text_;
};