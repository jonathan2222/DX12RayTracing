# TODO

- [ ] Start with implementing support for a normal DX12 pipleline
  - [x] Shaders.
  - [ ] shader hot reloading.
    - [ ] Allow includes to also be included in the hot shader reloading.
  - [x] Buffer resource class.
  - [x] Texture resource class.
  - [ ] Try using the DX12 residency manager.
  - [ ] Make the descriptor allocator be able to have null resources. (Atm it needs to match the root signature)
  - [ ] Make it automatically detect and copy the right dxc libs and dlls.

- [ ] DX12 Raytracing.
  - [ ] Add a new Pipleling for Raytracing.
  - [ ] Add the new shader types.
