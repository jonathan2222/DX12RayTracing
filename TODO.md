# TODO

- [ ] Start with implementing support for a normal DX12 pipleline
  - [x] Shaders.
  - [x] shader hot reloading.
    - [ ] Allow includes to also be included in the hot shader reloading.
  - [x] Buffer resource class.
  - [x] Texture resource class.
  - [ ] Try using the DX12 residency manager.
  - [ ] Make the descriptor allocator be able to have null resources. (Atm it needs to match the root signature)
  - [ ] Make it automatically detect and copy the right dxc libs and dlls.

- [ ] DX12 Raytracing.
  - [ ] Add a new Pipleling for Raytracing.
  - [ ] Add the new shader types.

## New goal

- [ ] Make compile times faster
  - [ ] Isolate windows specific code. Right now windows.h is included in Defines.h
  - [ ] Reduce number of header includes.
- [ ] Use the DX12 smaples MiniEngine as reference of how to create the backend for dx12 rendering.
- [ ] Remake how command list are used. Make them easier to understand when they are executed and how they are fetched and reset. What we want:
  - [ ] One command list of each type per thread (direct, compute, raytracing)
  - [ ] Use residency manager to keep GPU memory under control.
  - [ ] Use a render thread
