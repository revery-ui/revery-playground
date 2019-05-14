[![Build Status](https://dev.azure.com/revery-ui/revery/_apis/build/status/revery-ui.revery-playground?branchName=master)](https://dev.azure.com/revery-ui/revery/_build/latest?definitionId=9&branchName=master)

# revery-playground

## Build

### Prerequisites

- Make sure you have run `esy build` from the root directory

### Build

- `esy install`
- `esy build`
- `npm install`
- `npm run build`

### Testing

- `npm start`

## Code Structure

The Revery playground is split into two components:
- The _front-end_ (renderer) - responsible for rendering the UI
- The _back-end_ (worker) - responsible for parsing and compiling the code

The code is structured as follows:
- `src/lib/` - Common code between the renderer and worker, common types, and the communication protocol.
- `src/renderer` - The front-end renderer code. This is responsible for rendering, layout, and acquiring and passing UI events to the worker.
- `src/worker` - The back-end worker code. This is responsible for picking up code changes, parsing, and notifying the UI. It also runs the app loop (animations, ticker).


## Special Thanks

- [@thangngoc89](https://github.com/thangngoc89) for sketch.sh and his great blog series: https://khoanguyen.me/sketch/part-2-the-engine/ (and [sketch-v2](https://github.com/thangngoc89/sketch-v2) - which powers the latest builds of the playground).
- [Ocsigen](http://ocsigen.org) for the excellent [js_of_ocaml](https://github.com/ocsigen/js_of_ocaml) tool
- [Microsoft](https://microsoft.com) for the excellent [Monaco editor](https://microsoft.github.io/monaco-editor/index.html)
