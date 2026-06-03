#include "EventLogPanel.hpp"

#include "imgui.h"

namespace sw::graphics::ui {

void drawEventLogPanel(const EventFeed& feed, VisualizerState& state) {
    if (!state.showLog) {
        return;
    }

    ImGui::Begin("Event Log");

    if (ImGui::BeginChild("event_log_scroll", ImVec2(0.0F, 0.0F), ImGuiChildFlags_None)) {
        for (const auto& line : feed.lines()) {
            ImGui::TextUnformatted(line.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0F);
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

}  // namespace sw::graphics::ui
