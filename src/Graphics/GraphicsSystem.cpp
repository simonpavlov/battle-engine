#include "GraphicsSystem.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

#include "Picking.hpp"
#include "Renderer.hpp"
#include "WorldEditor.hpp"
#include "Ui/EventLogPanel.hpp"
#include "Ui/PropertiesPanel.hpp"
#include "Ui/Toolbar.hpp"
#include "Ui/UnitsPanel.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"

namespace sw::graphics {

namespace {

constexpr float kRotateSpeed = 0.005F;
constexpr float kZoomSpeed = 1.0F;
constexpr float kMinDistance = 2.0F;
constexpr float kMaxDistance = 80.0F;
constexpr float kPitchLimit = 1.5F;
constexpr float kEpsilon = 0.0001F;

void updateCamera(Camera3D& camera) {
    const Vector3 offset = Vector3Subtract(camera.position, camera.target);
    float distance = std::max(Vector3Length(offset), kEpsilon);

    float yaw = std::atan2(offset.x, offset.z);
    float pitch = std::asin(std::clamp(offset.y / distance, -1.0F, 1.0F));

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        const Vector2 delta = GetMouseDelta();
        yaw -= delta.x * kRotateSpeed;
        pitch += delta.y * kRotateSpeed;
        pitch = std::clamp(pitch, -kPitchLimit, kPitchLimit);
    }

    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0F) {
        distance -= wheel * kZoomSpeed;
        distance = std::clamp(distance, kMinDistance, kMaxDistance);
        ImGui::MarkIniSettingsDirty();
    }

    const float cosPitch = std::cos(pitch);
    camera.position = Vector3{
        camera.target.x + (distance * cosPitch * std::sin(yaw)),
        camera.target.y + (distance * std::sin(pitch)),
        camera.target.z + (distance * cosPitch * std::cos(yaw)),
    };
}

constexpr float kLeftColumnFraction = 0.22F;
constexpr float kRightColumnFraction = 0.30F;
constexpr float kEventLogFraction = 0.22F;

void buildDefaultLayout(ImGuiID dockspaceId) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

    ImGuiID remaining = dockspaceId;
    const ImGuiID left = ImGui::DockBuilderSplitNode(remaining, ImGuiDir_Left, kLeftColumnFraction, nullptr, &remaining);
    const ImGuiID right =
        ImGui::DockBuilderSplitNode(remaining, ImGuiDir_Right, kRightColumnFraction, nullptr, &remaining);
    const ImGuiID bottom =
        ImGui::DockBuilderSplitNode(remaining, ImGuiDir_Down, kEventLogFraction, nullptr, &remaining);

    ImGui::DockBuilderDockWindow("Units", left);
    ImGui::DockBuilderDockWindow("Properties", right);
    ImGui::DockBuilderDockWindow("Event Log", bottom);
    ImGui::DockBuilderFinish(dockspaceId);
}

class GraphicsSystem : public IGraphicsSystem {
public:
    GraphicsSystem(sw::core::Engine& engine, VisualizerState& state, EventFeed& feed, const IconSet& icons) :
            engine_(engine),
            state_(state),
            feed_(feed),
            icons_(icons) {}

    void frame() override {
        if (!ImGui::GetIO().WantCaptureMouse) {
            if (state_.placement != Placement::None) {
                handlePlacement();
            } else {
                updateCamera(state_.camera);
                handleSelectionAndDrag();
            }
        }

        worldEditor_.drain(engine_);

        if (state_.stepRequested) {
            engine_.step();
            state_.stepRequested = false;
        }

        if (state_.playing) {
            const double now = GetTime();
            const double interval = 1.0 / static_cast<double>(std::max(state_.speed, 0.1F));
            if (now - lastStepTime_ >= interval) {
                const bool advanced = engine_.step();
                lastStepTime_ = now;
                if (!advanced) {
                    state_.playing = false;
                }
            }
        }

        if (state_.hasSelection && !feed_.roster().contains(state_.selected)) {
            state_.hasSelection = false;
            if (state_.placement == Placement::SetDestination) {
                state_.placement = Placement::None;
            }
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);

        BeginMode3D(state_.camera);
        renderer_.draw3D(engine_, feed_, state_);
        EndMode3D();

        renderer_.drawOverlay(engine_, feed_, state_);

        rlImGuiBegin();
        ui::drawToolbar(engine_, state_, icons_);
        const ImGuiID dockspaceId =
            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        if (!layoutBuilt_) {
            buildDefaultLayout(dockspaceId);
            layoutBuilt_ = true;
        }
        ui::drawUnitsPanel(engine_, state_, feed_, worldEditor_);
        ui::drawPropertiesPanel(engine_, state_, feed_, worldEditor_);
        ui::drawEventLogPanel(feed_, state_);
        rlImGuiEnd();

        if (!state_.capturePath.empty()) {
            TakeScreenshot(state_.capturePath.c_str());
            state_.capturePath.clear();
        }

        EndDrawing();
    }

    bool shouldClose() override {
        return WindowShouldClose();
    }

    ~GraphicsSystem() override = default;

private:
    sw::core::components::Position boardSize() const {
        std::uint32_t width = feed_.boardWidth();
        std::uint32_t height = feed_.boardHeight();
        if (width == 0) {
            width = kFallbackBoardSize;
        }
        if (height == 0) {
            height = kFallbackBoardSize;
        }
        return sw::core::components::Position{width, height};
    }

    void handlePlacement() {
        const bool cancel = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE);
        const bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        const auto board = boardSize();
        const auto cell = pickCell(state_.camera, GetMousePosition(), board.x, board.y);

        switch (state_.placement) {
        case Placement::AddPosition:
            if (cancel) {
                state_.placement = Placement::None;
            } else if (clicked && cell) {
                state_.pendingPosition = cell;
                state_.placement = Placement::AddDestination;
            }
            break;
        case Placement::AddDestination:
            if (cancel) {
                if (state_.pendingPosition) {
                    ui::applyAddForm(worldEditor_, state_, *state_.pendingPosition, std::nullopt);
                }
                state_.pendingPosition.reset();
                state_.placement = Placement::None;
            } else if (clicked && cell && state_.pendingPosition) {
                ui::applyAddForm(worldEditor_, state_, *state_.pendingPosition, cell);
                state_.pendingPosition.reset();
                state_.placement = Placement::None;
            }
            break;
        case Placement::SetDestination:
            if (cancel) {
                state_.placement = Placement::None;
            } else if (clicked && cell && state_.hasSelection) {
                worldEditor_.setDestination(state_.selected, *cell);
                state_.placement = Placement::None;
            }
            break;
        case Placement::None:
            break;
        }
    }

    void handleSelectionAndDrag() {
        constexpr float kDragThreshold = 5.0F;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const auto picked = pickUnit(engine_, feed_, state_.camera, GetMousePosition());
            if (picked) {
                state_.selected = *picked;
                state_.hasSelection = true;
                state_.dragId = *picked;
                state_.dragStart = GetMousePosition();
                state_.dragging = false;
                pressedOnUnit_ = true;
            } else {
                state_.hasSelection = false;
                pressedOnUnit_ = false;
            }
        }

        if (pressedOnUnit_ && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            const Vector2 mouse = GetMousePosition();
            if (std::abs(mouse.x - state_.dragStart.x) > kDragThreshold ||
                std::abs(mouse.y - state_.dragStart.y) > kDragThreshold) {
                state_.dragging = true;
            }
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (pressedOnUnit_ && state_.dragging) {
                const auto board = boardSize();
                if (const auto cell = pickCell(state_.camera, GetMousePosition(), board.x, board.y)) {
                    worldEditor_.moveUnit(state_.dragId, *cell);
                }
            }
            state_.dragging = false;
            pressedOnUnit_ = false;
        }
    }

    sw::core::Engine& engine_;
    VisualizerState& state_;
    EventFeed& feed_;
    const IconSet& icons_;
    Renderer renderer_;
    WorldEditor worldEditor_;
    bool layoutBuilt_ = false;
    bool pressedOnUnit_ = false;
    double lastStepTime_ = 0.0;
};

}  // namespace

std::unique_ptr<IGraphicsSystem>
makeGraphicsSystem(sw::core::Engine& engine, VisualizerState& state, EventFeed& feed, const IconSet& icons) {
    return std::make_unique<GraphicsSystem>(engine, state, feed, icons);
}

}  // namespace sw::graphics
