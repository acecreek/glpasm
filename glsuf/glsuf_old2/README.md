# glsuf

MFC/OpenGL SUF viewer converted from the Delphi `glsuf2` sources.

## Source correspondence

- `SufModel.cpp` contains the `SufLoad` and `SufAtrLoad` equivalents.
- `GLView.cpp` contains the `SufPolygon`, `SufLine`, `SufPoint`, and `NormalVectorCalc` equivalents.
- ATR material handling follows the active `Suf.pas` `AtrSet` behavior, including DoGA transparency conversion as `alpha = 1 - tra`.

## Usage

1. Build `glsuf.sln` in Visual Studio.
2. Use `File > Open ATR...` to load `GENIE.ATR` when material colors are needed.
3. Use `File > Open SUF...` to load a `.suf` model.
4. Use `View` or keyboard shortcuts:
   - `1`: polygon
   - `2`: line
   - `3`: point
   - `4`: polygon + line + point
   - `G`: grid
   - `X`: axes
   - `F`: frame all

Mouse operation follows the Delphi viewer closely: right drag rotates, Shift + right drag changes distance, and the wheel zooms.

