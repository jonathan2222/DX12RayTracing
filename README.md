# Sandbox Project for DirectX12

This is a sandbox project for me to learn about DirectX 12.

## Build
**Only supported Windows (as you might know) and Visual Studio 2019/2022**
1. Clone project
2. Run UpdateExternals.bat
3. Might need to go into Externals/ImGui and change to the "docking" branch if it is not already that.
4. This project uses the DirectX Shader Compiler thus would need the dxcompiler.lib, dxcompiler.dll and dxil.dll.
    To get the right one, you can go into **../Windows Kits/<os number, 10/11>/bin/<version>/x64** and copy the relavent files into **Externals/dxc/**. 