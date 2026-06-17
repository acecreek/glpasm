# glpasm

MFC/OpenGL prototype for a DoGA-L3 style parts assembler.

## Current scope

- Single perspective OpenGL view. The old three-view layout is intentionally not recreated.
- Blender-like viewport navigation:
  - Middle mouse: orbit
  - Shift + middle mouse: pan
  - Mouse wheel: zoom
  - `G`: grid
  - `X`: axes
  - `W`: wireframe
  - `F`: frame all
- Right-side part list populated from `.l3p`, `.l2p`, or `.fsc` files.
- Basic parser for the Delphi `DogacgaFile.pas` flow: `mov`, rotation, `scal`, `obj`, and quoted SUF path.
- OpenGL rendering currently draws transformed placeholder boxes for each parsed part.

## Next implementation steps

1. Port the SUF loader from the Pascal `glpasm` / `glsuf` code into C++.
2. Replace placeholder boxes in `CGLView::DrawPart` with real SUF mesh rendering.
3. Add material palette editing and assign the material list to loaded polygons.
4. Add transform gizmo behavior for selected parts.
5. Persist window/camera settings in an INI file compatible with the existing tool chain where useful.

## Reference sources

- `D:\acecreek\DoGA_Dev\Pascal\glpasm`
- `D:\acecreek\DoGA_Dev\Pascal\glsuf`
- `D:\acecreek\DoGA_Dev\MFC\Delphi2MFC_GL`
