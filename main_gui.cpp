#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GL_SILENCE_DEPRECATION
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <algorithm>
#include <cmath>
#include <array>

using namespace std;

// =============================================================================
// [1] CORE MEMORY LOGIC
// =============================================================================
namespace MemSim {
    struct Hole {
        int start_address;
        int size;
        bool operator<(const Hole& other) const { return start_address < other.start_address; }
    };

    struct Segment {
        string name;
        int size;
        int base_address = -1;
    };

    struct Process {
        string name;
        vector<Segment> segments;
        bool is_allocated = false;
        bool is_failed = false; 
    };

    static int total_memory_size = 1000;
    static list<Hole> free_holes = {{0, 1000}};
    static vector<Process> processes;
    static int algo_index = 0; 
    static const char* kAlgorithmLabels[] = { "First-Fit", "Best-Fit" };

    // Standard distinct color generator for processes
    static ImU32 getProcessColor(int procIndex) {
        if (procIndex < 0) return IM_COL32(80, 80, 85, 255); // Free hole color (Grey)
        float h = (procIndex * 0.618033988749895f); 
        h = h - std::floor(h);
        ImVec4 c = ImColor::HSV(h, 0.65f, 0.85f);
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    void mergeHoles() {
        free_holes.sort();
        auto it = free_holes.begin();
        while (it != free_holes.end()) {
            auto next_it = next(it);
            if (next_it != free_holes.end() && it->start_address + it->size == next_it->start_address) {
                it->size += next_it->size;
                free_holes.erase(next_it);
            } else {
                ++it;
            }
        }
    }

    void allocateProcess(Process& p) {
        list<Hole> temp_holes = free_holes;
        p.is_failed = false;

        for (auto& seg : p.segments) {
            auto best_hole_it = temp_holes.end();

            if (algo_index == 0) { // First-Fit
                for (auto it = temp_holes.begin(); it != temp_holes.end(); ++it) {
                    if (it->size >= seg.size) { best_hole_it = it; break; }
                }
            } else { // Best-Fit
                int min_diff = -1;
                for (auto it = temp_holes.begin(); it != temp_holes.end(); ++it) {
                    if (it->size >= seg.size) {
                        int diff = it->size - seg.size;
                        if (min_diff == -1 || diff < min_diff) {
                            min_diff = diff;
                            best_hole_it = it;
                        }
                    }
                }
            }

            if (best_hole_it == temp_holes.end()) {
                p.is_failed = true; 
                return; 
            }

            seg.base_address = best_hole_it->start_address;
            best_hole_it->start_address += seg.size;
            best_hole_it->size -= seg.size;
            if (best_hole_it->size == 0) temp_holes.erase(best_hole_it);
        }

        free_holes = temp_holes; 
        p.is_allocated = true;
    }

    void deallocateProcess(int p_idx) {
        Process& p = processes[p_idx];
        if (!p.is_allocated) return;
        
        for (auto& seg : p.segments) {
            free_holes.push_back({seg.base_address, seg.size});
            seg.base_address = -1;
        }
        p.is_allocated = false;
        mergeHoles();
    }
    
    void resetMemory(bool clear_custom_holes = true) {
        if (clear_custom_holes) {
            free_holes.clear();
            free_holes.push_back({0, total_memory_size});
        }
        processes.clear();
    }
}

// =============================================================================
// [2] UNIFIED SINGLE-SCREEN UI
// =============================================================================
void DrawMemoryAllocatorUI() {
    using namespace MemSim;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    
    // Main background window
    ImGui::Begin("Memory Allocator Interface", nullptr, 
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

    float avail_y = ImGui::GetContentRegionAvail().y;
    float top_half_height = avail_y * 0.45f; // Top 45% for controls

    // -------------------------------------------------------------------------
    // TOP HALF: HARDWARE & PROCESS DIRECTIVES
    // -------------------------------------------------------------------------
    ImGui::BeginChild("TopHalf", ImVec2(0, top_half_height), false);
    ImGui::Columns(2, "TopColumns", false);
    
    // --- TOP LEFT: HARDWARE OVERRIDE ---
    ImGui::BeginChild("HardwarePanel", ImVec2(-1, -1), true);
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "HARDWARE SETTINGS");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Total Memory Size:");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputInt("##totmem", &total_memory_size, 100, 1000)) {
        if (total_memory_size < 10) total_memory_size = 10;
        resetMemory(true);
    }
    ImGui::Spacing();
    
    ImGui::Text("Allocation Algorithm:");
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##algo", &algo_index, kAlgorithmLabels, IM_ARRAYSIZE(kAlgorithmLabels));
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("h",ImVec2(-1,-50),false);
    ImGui::Text("Define Initial Holes");
    static int h_start = 0, h_size = 100;
    ImGui::Text("start"); ImGui::SameLine();
    ImGui::SetNextItemWidth(120); ImGui::InputInt("##Start##h", &h_start,10); ImGui::SameLine();
    ImGui::Text("size"); ImGui::SameLine();
    ImGui::SetNextItemWidth(120); ImGui::InputInt("##Size##h", &h_size,10);
    ImGui::EndChild();
    if (ImGui::Button("Add Hole", ImVec2(-1, 20))) {
        if (h_size > 0 && h_start >= 0) {
            if (free_holes.size() == 1 && free_holes.front().size == total_memory_size) free_holes.clear();
            free_holes.push_back({h_start, h_size});
            mergeHoles();
            h_start += h_size; 
        }
    }
    if (ImGui::Button("Reset Memory Layout", ImVec2(-1, 20))) {
        resetMemory(true);
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    // --- TOP RIGHT: PROCESS DEPLOYMENT ---
    ImGui::BeginChild("ProcessPanel", ImVec2(0, 0), true);
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "PROCESS SETUP");
    ImGui::Separator();
    ImGui::Spacing();

    // Allocate Process Form 
    static char p_name[32] = "P1";
    static int num_segs = 1;
    static vector<std::array<char, 32>> seg_names(1);
    static vector<int> seg_sizes(1, 50);

    ImGui::Text("Process name");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::InputTextWithHint("##pname", "Process Name", p_name, IM_ARRAYSIZE(p_name));
    ImGui::Text("Segment nums");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    if (ImGui::InputInt("##Segments", &num_segs)) {
        if (num_segs < 1) num_segs = 1;
        if (num_segs > 8) num_segs = 8;
        seg_names.resize(num_segs, {0});
        seg_sizes.resize(num_segs, 50);
    }

    // Scrollable segment builder
    ImGui::BeginChild("SegBuilder", ImVec2(-1, -35), false);
    for (int i = 0; i < num_segs; i++) {
        ImGui::PushID(i);
        ImGui::Text("Segment name");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::InputText("##sname", seg_names[i].data(), 32);
        ImGui::SameLine();
        ImGui::Text("Segment size");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("##Size", &seg_sizes[i]);
        ImGui::PopID();
    }
    
    ImGui::EndChild();
   if (ImGui::Button("Allocate Process", ImVec2(-1, 30))) {
        if (strlen(p_name) == 0) {
            // Trigger the name error popup
            ImGui::OpenPopup("Error: Empty Name");
        } else {
            Process new_p;
            new_p.name = p_name;
            for (int i = 0; i < num_segs; i++) {
                if (strlen(seg_names[i].data()) > 0 && seg_sizes[i] > 0) {
                    new_p.segments.push_back({seg_names[i].data(), seg_sizes[i], -1});
                }
            }
            
            if (new_p.segments.empty()) {
                // Trigger the segment error popup if no valid segments were parsed
                ImGui::OpenPopup("Error: Invalid Segments");
            } else {
                // Everything is valid, proceed with allocation
                allocateProcess(new_p);
                processes.push_back(new_p);
                // CHECK IF IT FAILED HERE:
                if (processes.back().is_failed) {
                    ImGui::OpenPopup("Error: Space Issue");
                }
                // Auto-increment name
                string next_name = "P" + to_string(processes.size() + 1);
                strncpy(p_name, next_name.c_str(), sizeof(p_name));
            }
        }
    }

    // --------------------------------------------------------
    // POPUP DEFINITIONS (Must stay outside the Button block)
    // --------------------------------------------------------
    
    // 1. Popup for Empty Name
    if (ImGui::BeginPopupModal("Error: Empty Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Allocation Failed!");
        ImGui::Text("You must assign a name for the process.");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 2. Popup for Invalid/Empty Segments
    if (ImGui::BeginPopupModal("Error: Invalid Segments", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Allocation Failed!");
        ImGui::Text("You must provide at least one valid segment.");
        ImGui::TextDisabled("(Segment names cannot be empty, and sizes must be > 0)");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    // 3. Popup for Space Issue
    if (ImGui::BeginPopupModal("Error: Space Issue", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Allocation Failed!");
        ImGui::Text("The process does not fit in the available memory holes.");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    ImGui::EndChild();
    
    ImGui::Columns(1);
    ImGui::EndChild(); // End TopHalf

    ImGui::Spacing();

    // -------------------------------------------------------------------------
    // BOTTOM HALF: SECTOR VISUALIZATION & PAGE TABLES
    // -------------------------------------------------------------------------
    ImGui::BeginChild("BottomHalf", ImVec2(0, 0), true);
    
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "MEMORY MAP");
    ImGui::Separator();
    ImGui::Spacing();

    // -- 1. Visual RAM Bar --
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    float avail_w = ImGui::GetContentRegionAvail().x - 10.f;
    float map_h = 60.f; 
    float scale = (total_memory_size > 0) ? (avail_w / static_cast<float>(total_memory_size)) : 1.0f;

    // Dark background represents hardware limit
    dl->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + avail_w, canvas_pos.y + map_h), IM_COL32(30, 30, 35, 255));
    dl->AddRect(canvas_pos, ImVec2(canvas_pos.x + avail_w, canvas_pos.y + map_h), IM_COL32(100, 100, 100, 255), 0.f, 0, 1.f);

    struct RenderBlock { int start; int size; string label; int proc_idx; int end; };
    vector<RenderBlock> blocks;
    
    // FIX 1: Added the 'end' calculation (start_address + size) to the free holes
    for (const auto& h : free_holes) {
        blocks.push_back({h.start_address, h.size, "Free", -1, h.start_address + h.size});
    }
    
    for (int i = 0; i < (int)processes.size(); i++) {
        if (!processes[i].is_allocated) continue;
        for (const auto& s : processes[i].segments) {
            // FIX 2: End address is base_address + size (NOT minus)
            blocks.push_back({s.base_address, s.size, processes[i].name + ":" + s.name, i, s.base_address + s.size});
        }
    }
    
    int bs = 0;
    
    // Draw Blocks
    for (const auto& b : blocks) {
        if (b.start >= total_memory_size || b.size <= 0) continue;
        float x0 = canvas_pos.x + (b.start * scale);
        float x1 = canvas_pos.x + ((b.start + b.size) * scale);
        if (x1 > canvas_pos.x + avail_w) x1 = canvas_pos.x + avail_w;

        ImU32 col = getProcessColor(b.proc_idx);
        dl->AddRectFilled(ImVec2(x0, canvas_pos.y), ImVec2(x1, canvas_pos.y + map_h), col);
        dl->AddRect(ImVec2(x0, canvas_pos.y), ImVec2(x1, canvas_pos.y + map_h), IM_COL32(200,200, 200, 255), 0.f, 0, 4.f); // Inner borders
        
        // Centered Text Label
        float seg_w = x1 - x0;
        if (seg_w > 40.f) {
            ImVec2 ts = ImGui::CalcTextSize(b.label.c_str());
            float tx = x0 + (seg_w - ts.x) * 0.5f;
            float ty = canvas_pos.y + (map_h - ts.y) * 0.5f;
            dl->AddText(ImVec2(tx, ty), IM_COL32(255, 255, 255, 255), b.label.c_str()); 
        }
        bs += b.size;
        
        // --- DRAW ADDRESS MARKERS UNDER THE BLOCKS ---
        
        // 1. Draw Start Address (Centered on the left line)
        string start_str = to_string(b.start);
        if(b.start==0) continue;
        ImVec2 start_ts = ImGui::CalcTextSize(start_str.c_str());
        dl->AddText(ImVec2(x0 - (start_ts.x * 0.5f), canvas_pos.y + map_h + 4.f), IM_COL32(200, 200, 200, 255), start_str.c_str());

        // 2. Draw End Address (Centered on the right line)
        string end_str = to_string(b.end);
        if(b.end==total_memory_size) continue;
        ImVec2 end_ts = ImGui::CalcTextSize(end_str.c_str());
        dl->AddText(ImVec2(x1 - (end_ts.x * 0.5f), canvas_pos.y + map_h + 4.f), IM_COL32(200, 200, 200, 255), end_str.c_str());
    }
    
    ImGui::Dummy(ImVec2(avail_w, map_h + 5.f));
    ImVec2 axis_pos = ImGui::GetCursorScreenPos();
    dl->AddText(axis_pos, IM_COL32(180, 180, 180, 255), "0");
    
    
    string end_label = to_string(total_memory_size);
    ImVec2 end_ts = ImGui::CalcTextSize(end_label.c_str());
    dl->AddText(ImVec2(axis_pos.x + avail_w - end_ts.x, axis_pos.y), IM_COL32(180, 180, 185, 255), end_label.c_str());
    
    ImGui::Dummy(ImVec2(0, 20.f));
    ImGui::Separator();

    // -- 2. Segment & Process Matrix (Splitting the remaining bottom area) --
    ImGui::Columns(2, "MatrixColumns", false);
    
    // Active Processes List
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "ACTIVE PROCESSES");
    ImGui::BeginChild("ActiveProcesses", ImVec2(0, 0), true);
    if (processes.empty()) ImGui::TextDisabled("No processes generated.");
    
    for (int i = 0; i < (int)processes.size(); ++i) {
        ImGui::PushID(i);
        if (processes[i].is_allocated) {
            ImGui::TextColored(ImVec4(0.40f, 1.00f, 0.40f, 1.0f), "[Allocated] %s", processes[i].name.c_str());
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 70);
            if (ImGui::Button("Deallocate")) deallocateProcess(i);
        } else if (processes[i].is_failed) {
            ImGui::TextColored(ImVec4(1.00f, 0.30f, 0.30f, 1.0f), "[Error]  %s (Does Not Fit)", processes[i].name.c_str());
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 70);
            if (ImGui::Button("Remove")) { processes.erase(processes.begin() + i); i--; }
        } else {
            ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.70f, 1.0f), "[Removed] %s", processes[i].name.c_str());
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 70);
            if (ImGui::Button("Remove")) { processes.erase(processes.begin() + i); i--; }
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    // Segment Page Tables
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "SEGMENT TABLES");
    if (ImGui::BeginTable("metrics_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 0))) {
        ImGui::TableSetupColumn("Process / Segment", ImGuiTableColumnFlags_WidthStretch, 0.4f);
        ImGui::TableSetupColumn("Limit (Size)", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("Base Address", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (int p_idx = 0; p_idx < (int)processes.size(); p_idx++) {
            const auto& p = processes[p_idx];
            if (!p.is_allocated) continue;
            
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            ImGui::TableSetColumnIndex(0); 
            ImVec4 p_color = ImGui::ColorConvertU32ToFloat4(getProcessColor(p_idx));
            ImGui::Bullet();
            ImGui::TextColored(p_color, "%s", p.name.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("-"); 
            ImGui::TableSetColumnIndex(2); ImGui::Text("-");

            for (const auto& s : p.segments) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("  -> %s", s.name.c_str());
                ImGui::TableSetColumnIndex(1); ImGui::Text("%d", s.size);
                ImGui::TableSetColumnIndex(2); ImGui::Text("%d", s.base_address);
            }
        }
        ImGui::EndTable();
    }

    ImGui::Columns(1);
    ImGui::EndChild(); // End BottomHalf
    
    ImGui::End(); // End Main Window
}

// =============================================================================
// [3] MAIN ENTRY POINT (Cross-Platform / MinGW Safe)
// =============================================================================
static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}


int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Memory Allocator GUI", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Standard ImGui Dark Theme for best contrast
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.ScaleAllSizes(1.3f); 

    ImFontConfig font_cfg;
    font_cfg.SizePixels = 16.0f;
    io.Fonts->AddFontDefault(&font_cfg);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.11f, 1.00f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Shutdown;
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        DrawMemoryAllocatorUI();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}