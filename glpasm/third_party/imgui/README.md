ImGui placeholder

This project expects Dear ImGui sources to be placed under third_party/imgui when you enable IMGUI integration.

Steps to add ImGui:
1. Clone or copy Dear ImGui sources into third_party/imgui (from https://github.com/ocornut/imgui).
   Required files (at minimum):
	 - imgui.h, imgui.cpp, imgui_draw.cpp, imgui_tables.cpp, imgui_widgets.cpp
	 - backends/imgui_impl_win32.h/.cpp
	 - backends/imgui_impl_opengl2.h/.cpp

2. The project file glsuf.vcxproj is configured to include third_party/imgui\**\*.cpp automatically.
   After placing the files, build the solution.

3. The build uses the preprocessor macro IMGUI_ENABLED. If you prefer manual control, define/undefine IMGUI_ENABLED in the project properties.

If you want me to add ImGui sources directly into the repo, reply and I will add them. (Note: ImGui is third-party open source; consider adding as a git submodule.)