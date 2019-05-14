open Revery;
open Revery.Draw;
open Revery.Math;
open Revery.UI;
open Js_of_ocaml;
open PlaygroundLib;
open PlaygroundLib.Types;

let log = (msg) => print_endline ("[Compiler] " ++ msg);
let error = (msg) => prerr_endline ("[Compiler] " ++ msg);

let rootNode: ref(option(viewNode)) = ref(None);

let createNode = nodeType =>
  switch (nodeType) {
  | View => (new viewNode)()
  | Text => Obj.magic((new textNode)(""))
  | Image => Obj.magic((new imageNode)(""))
  /* | _ => (new viewNode)() */
  };

let idToNode: Hashtbl.t(int, node) = Hashtbl.create(100);
let idToWorkerId: Hashtbl.t(int, int) = Hashtbl.create(100);

let nodeFromId = (id: int) => {
  let item = Hashtbl.find_opt(idToNode, id);
  switch (item) {
  | Some(v) => v
  | None =>
    failwith(
      "(renderer - nodeFromId) Cannot find node: " ++ string_of_int(id),
    )
  };
};

let workerIdFromId = (id: int) => {
  let workerId = Hashtbl.find_opt(idToWorkerId, id);
  switch (workerId) {
  | Some(v) => v
  | None =>
    failwith(
      "(renderer - workerIdFromId) Cannot find node: " ++ string_of_int(id),
    )
  };
};

let rec buildMeasurements = (node: viewNode) => {
  let nodeToMeasurement = (node: viewNode) => {
    let ret: Protocol.ToWorker.nodeMeasurement = {
      id: workerIdFromId(node#getInternalId()),
      dimensions: node#measurements(),
    };
    ret;
  };

  let childMeasurements =
    node#getChildren() |> List.map(buildMeasurements) |> List.flatten;

  [nodeToMeasurement(node), ...childMeasurements];
};

let visitUpdate = u =>
  switch (u) {
  | RootNode(id) =>
    let node = nodeFromId(id);
    rootNode := Obj.magic(Some(node));
    Hashtbl.add(idToNode, id, node);
    Hashtbl.add(idToWorkerId, node#getInternalId(), id);
  | NewNode(id, nodeType) =>
    let node = createNode(nodeType);
    Hashtbl.add(idToNode, id, node);
    Hashtbl.add(idToWorkerId, node#getInternalId(), id);
  | SetStyle(id, style) =>
    let node = nodeFromId(id);
    node#setStyle(style);
  | AddChild(parentId, childId) =>
    let parentNode = nodeFromId(parentId);
    let childNode = nodeFromId(childId);
    parentNode#addChild(childNode);
  | RemoveChild(parentId, childId) =>
    let parentNode = nodeFromId(parentId);
    let childNode = nodeFromId(childId);
    parentNode#removeChild(childNode);
  | SetText(id, text) =>
    let textNode = Obj.magic(nodeFromId(id));
    let _ = textNode#setText(text);
    ();
  | SetImageSrc(id, src) =>
    let imageNode = Obj.magic(nodeFromId(id));
    let _ = imageNode#setSrc(src);
    ();
  | SetImageResizeMode(id, rm) =>
    let imageNode = Obj.magic(nodeFromId(id));
    let _ = imageNode#setResizeMode(rm);
    ();
  | _ => ()
  };

let update = (v: list(updates)) => {
  /* Types.showAll(v); */
  List.iter(
    visitUpdate,
    v,
  );
};

let start =
    (
      onCompiling,
      onReady,
      onOutput,
      onSyntaxChanged,
      onError,
      onCompilationResult,
    ) => {
  let isWorkerReady = ref(false);
  let latestSourceCode: ref(option(Js.t(Js.js_string))) = ref(None);

  let isLayoutDirty = ref(false);

  let worker = Js_of_ocaml.Worker.create("./worker.js");

  let sendLatestSource = () => {
    switch (latestSourceCode^) {
    | Some(v) => worker##postMessage(Protocol.ToWorker.SourceCodeUpdated(v))
    | None => ()
    };

    latestSourceCode := None;
  };

  let setSyntax = (v: Js.t(Js.js_string)) => {
    let str = Js.to_string(v) |> String.lowercase;
    switch (str) {
    | "ml" =>
      worker##postMessage(Protocol.ToWorker.SetSyntax(Protocol.Syntax.ML))
    | "re" =>
      worker##postMessage(Protocol.ToWorker.SetSyntax(Protocol.Syntax.RE))
    | _ => prerr_endline("setSyntax: Unknown syntax - " ++ str)
    };
  };

  let sendMeasurements = () => {
    switch (rootNode^) {
    | None => ()
    | Some(v) =>
      let measurements = buildMeasurements(v);
      worker##postMessage(Protocol.ToWorker.Measurements(measurements));
    };
  };

  let sendMouseEvent = mouseEvent => {
    worker##postMessage(Protocol.ToWorker.MouseEvent(mouseEvent));
  };

  let sendKeyboardEvent = keyboardEvent => {
    worker##postMessage(Protocol.ToWorker.KeyboardEvent(keyboardEvent));
  };

  let handleMessage = (msg: Protocol.ToRenderer.t) => {
    switch (msg) {
    | Updates(updates) =>
      isLayoutDirty := true;
      update(updates);
    | Output(v) =>
      let _ = Js.Unsafe.fun_call(onOutput, [|Obj.magic(v)|]);
      ();
    | PhraseResult(v) =>
      switch (v) {
      | Directive(_) => ()
      | Phrase({blockLoc, blockContent, evalId}) =>
        switch (blockContent) {
        | BlockStart => ()
        | BlockSuccess(_) => ()
        | BlockError({error, _}) =>
          let jsError =
            error
            |> MonacoDiagnostics.errorToDiagnostic
            |> MonacoDiagnostics.toJs;
          Js.Unsafe.fun_call(onError, [|Obj.magic(jsError)|]);
        };

        switch (blockLoc) {
        | None => ()
        | Some(v) =>
          open Core.Loc;
          let startLine = v.locStart.line;
          let endLine = v.locEnd.line;
		  let startCol = v.locStart.col;
		  let endCol = v.locEnd.col;

          let js =
            PhraseCompilationResult.toJs(
              evalId,
              startLine,
														startCol,
              endLine,
														endCol,
              blockContent,
            );
          Js.Unsafe.fun_call(onCompilationResult, [|Obj.magic(js)|]);
        };
      }
    | CompilationResult(v) =>
      switch (v) {
      | Core.Evaluate.EvalSuccess(evalId) =>
        log("Compilation success: " ++ string_of_int(evalId))
      | Core.Evaluate.EvalError(evalId) =>
        error("Compilation error: " ++ string_of_int(evalId))
      | Core.Evaluate.EvalInterupted(_) =>
        error("Compilation interrupted")
      }
    | Compiling(evalId) =>
      isWorkerReady := false;
      log("Compiling: " ++ string_of_int(evalId));
      let _ = Js.Unsafe.fun_call(onCompiling, [|Obj.magic(evalId)|]);
      ();
    | Ready =>
      isWorkerReady := true;
      print_endline("Ready!");
      let _ = Js.Unsafe.fun_call(onReady, [||]);
      print_endline("Ready called!");
      sendLatestSource();
      print_endline("Send latest source called!");
    | SyntaxChanged(newCode) =>
      let _ = Js.Unsafe.fun_call(onSyntaxChanged, [|Obj.magic(newCode)|]);
      ();
    | _ => ()
    };
  };

  worker##.onmessage :=
    Js_of_ocaml.Dom_html.handler(evt => {
      let data = Js.Unsafe.get(evt, "data");
      handleMessage(data);
      Js._true;
    });

  let ret = update => {
    latestSourceCode := Some(update);
    if (isWorkerReady^) {
      sendLatestSource();
    };
  };

  let init = app => {
    let win =
      App.createWindow(
        app,
        "Welcome to Revery",
        ~createOptions={
          WindowCreateOptions.create(~maximized=true, ());
        },
      );

    let _ =
      Revery_Core.Event.subscribe(
        win.onMouseMove,
        m => {
          let scaleFactor = Window.getScaleFactor(win);
          Revery_Core.Events.InternalMouseMove({
            mouseX: m.mouseX /. float_of_int(scaleFactor),
            mouseY: m.mouseY /. float_of_int(scaleFactor),
          })
          |> sendMouseEvent;
        },
      );
    let _ =
      Revery_Core.Event.subscribe(Revery_Draw.FontCache.onFontLoaded, () =>
        isLayoutDirty := true
      );

    let _ =
      Revery_Core.Event.subscribe(win.onMouseDown, m =>
        Revery_Core.Events.InternalMouseDown({button: m.button})
        |> sendMouseEvent
      );

    let _ =
      Revery_Core.Event.subscribe(win.onKeyPress, event =>
        Revery_Core.Events.InternalKeyPressEvent(event) |> sendKeyboardEvent
      );

    let _ =
      Revery_Core.Event.subscribe(win.onKeyDown, event =>
        Revery_Core.Events.InternalKeyDownEvent(event) |> sendKeyboardEvent
      );

    let _ =
      Revery_Core.Event.subscribe(win.onKeyUp, event =>
        Revery_Core.Events.InternalKeyUpEvent(event) |> sendKeyboardEvent
      );

    let _ =
      Revery_Core.Event.subscribe(win.onMouseUp, m =>
        Revery_Core.Events.InternalMouseUp({button: m.button})
        |> sendMouseEvent
      );

    let _ =
      Revery_Core.Event.subscribe(win.onMouseWheel, m =>
        Revery_Core.Events.InternalMouseWheel(m) |> sendMouseEvent
      );
    let _projection = Mat4.create();

    let render = () =>
      switch (rootNode^) {
      | None => ()
      | Some(rootNode) =>
        let size = Window.getSize(win);
        let pixelRatio = Window.getDevicePixelRatio(win);
        let scaleFactor = Window.getScaleFactor(win);
        let adjustedHeight = size.height / scaleFactor;
        let adjustedWidth = size.width / scaleFactor;

        rootNode#setStyle(
          Style.make(
            ~position=LayoutTypes.Relative,
            ~width=adjustedWidth,
            ~height=adjustedHeight,
            (),
          ),
        );

        /* let layoutNode = rootNode#toLayoutNode(~force=false, ()); */
        /* Layout.printCssNode(layoutNode); */

        if (isLayoutDirty^) {
          Layout.layout(~force=true, rootNode);
          rootNode#recalculate();
          sendMeasurements();
          isLayoutDirty := false;
        };

        Mat4.ortho(
          _projection,
          0.0,
          float_of_int(adjustedWidth),
          float_of_int(adjustedHeight),
          0.0,
          1000.0,
          -1000.0,
        );

        let drawContext = NodeDrawContext.create(~zIndex=0, ~opacity=1.0, ());

        /* Render all geometry that requires an alpha */
        RenderPass.startAlphaPass(
          ~pixelRatio,
          ~scaleFactor,
          ~screenHeight=adjustedHeight,
          ~screenWidth=adjustedWidth,
          ~projection=_projection,
          (),
        );
        rootNode#draw(drawContext);
        RenderPass.endAlphaPass();
      /* (renderFunction^)(); */
      };

    Window.setRenderCallback(win, render);
    Window.setShouldRenderCallback(win, () => true);

    Window.maximize(win);
    /* UI.start(win, render); */
  };

  App.start(init);

  Js.array([|ret, setSyntax|]);
};

let () = Js.export_all([%js {val startRenderer = start}]);
