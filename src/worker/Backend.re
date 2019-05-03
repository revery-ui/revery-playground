open Revery;
open Revery.UI;

open PlaygroundLib;
open PlaygroundLib.Types;

open Js_of_ocaml;

let log = v => print_endline("[Worker] " ++ v);

let renderFunction =
  ref(() =>
    <View
      style=Style.[backgroundColor(Colors.red), width(100), height(100)]
    />
  );

let _pendingUpdates: ref(list(updates)) = ref([]);
let clearUpdates = () => _pendingUpdates := [];
let queueUpdate = (update: updates) => {
  _pendingUpdates := [update, ..._pendingUpdates^];
};

class proxyViewNode (()) = {
  as _this;
  inherit (class viewNode)() as super;
  pub! setStyle = style => {
    queueUpdate(SetStyle(super#getInternalId(), style));
  };
  pub! addChild = child => {
    queueUpdate(AddChild(super#getInternalId(), child#getInternalId()));
    super#addChild(child);
  };
  pub! removeChild = child => {
    queueUpdate(RemoveChild(super#getInternalId(), child#getInternalId()));
    super#removeChild(child);
  };
  initializer {
    queueUpdate(NewNode(super#getInternalId(), View));
  };
};

class proxyTextNode (text) = {
  as _this;
  inherit (class textNode)(text) as super;
  pub! setStyle = style => {
    queueUpdate(SetStyle(super#getInternalId(), style));
  };
  pub! addChild = child => {
    queueUpdate(AddChild(super#getInternalId(), child#getInternalId()));
    super#addChild(child);
  };
  pub! removeChild = child => {
    queueUpdate(RemoveChild(super#getInternalId(), child#getInternalId()));
    super#removeChild(child);
  };
  pub! setText = text => {
    queueUpdate(SetText(super#getInternalId(), text));
  };
  initializer {
    queueUpdate(NewNode(super#getInternalId(), Text));
    queueUpdate(SetText(super#getInternalId(), text));
  };
};

class proxyImageNode (src) = {
  as _this;
  inherit (class imageNode)(src) as super;
  pub! setStyle = style => {
    queueUpdate(SetStyle(super#getInternalId(), style));
  };
  pub! addChild = child => {
    queueUpdate(AddChild(super#getInternalId(), child#getInternalId()));
    super#addChild(child);
  };
  pub! removeChild = child => {
    queueUpdate(RemoveChild(super#getInternalId(), child#getInternalId()));
    super#removeChild(child);
  };
  pub! setSrc = src => {
    queueUpdate(SetImageSrc(super#getInternalId(), src));
    super#setSrc(src);
  };
  initializer {
    queueUpdate(NewNode(super#getInternalId(), Image));
    queueUpdate(SetImageSrc(super#getInternalId(), src));
  };
};

let rootNode = (new proxyViewNode)();
queueUpdate(RootNode(rootNode#getInternalId()));
let container = ref(Container.create(rootNode));

let idToNode: Hashtbl.t(int, viewNode) = Hashtbl.create(100);

let registerNode = node => {
  Hashtbl.add(idToNode, node#getInternalId(), Obj.magic(node));
};
registerNode(rootNode);

let getNodeById = id => {
  switch (Hashtbl.find_opt(idToNode, id)) {
  | Some(v) => v
  | None => failwith("Unable to find node with id: " ++ string_of_int(id))
  };
};

let proxyNodeFactory: Revery.UI.Internal.PrimitiveNodeFactory.nodeFactory = {
  createViewNode: () => {
    let ret = (new proxyViewNode)();
    registerNode(ret);
    ret;
  },
  createTextNode: text => {
    let ret = (new proxyTextNode)(text);
    registerNode(ret);
    ret;
  },
  createImageNode: src => {
    let ret = (new proxyImageNode)(src);
    registerNode(ret);
    ret;
  },
};

Revery.UI.Internal.PrimitiveNodeFactory.set(proxyNodeFactory);

let sendMessage = msg => {
  Worker.post_message(msg);
};

let dirty = ref(false);

let render = () => {
  container := Container.update(container^, renderFunction^());
  let updatesToSend = _pendingUpdates^ |> List.rev;
  sendMessage(Protocol.ToRenderer.Updates(updatesToSend));
  clearUpdates();
};

let onStale = () => {
  log("onStale - re-rendering");
  dirty := true;
};

let _ = Revery_Core.Event.subscribe(React.onStale, onStale);

let latestCode = ref(None);

let setRenderFunction = fn => {
  renderFunction := fn;
  dirty := true;
  /* render(); */
};

let start = exec => {
  let mouseCursor = Revery_UI.Mouse.Cursor.make();

  Worker.set_onmessage((updates: Protocol.ToWorker.t) =>
    switch (updates) {
    | SourceCodeUpdated(v) =>
      log("got source code update");
      sendMessage(Protocol.ToRenderer.Compiling);
      let code = Js.to_string(v);
      latestCode := Some(code);
      let send = r => sendMessage(Protocol.ToRenderer.PhraseResult(r));
      let complete = r =>
        sendMessage(Protocol.ToRenderer.CompilationResult(r));

      let output = Obj.magic(exec(~send, ~complete, code));
      sendMessage(Protocol.ToRenderer.Output(output));
      sendMessage(Protocol.ToRenderer.Ready);
    | Measurements(v) =>
      let f = (measurement: Protocol.ToWorker.nodeMeasurement) => {
        let nodeId = measurement.id;
        let measurements = measurement.dimensions;

        let node = getNodeById(nodeId);
        node#forceMeasurements(measurements);
      };

      List.iter(f, v);
      rootNode#recalculate();
      rootNode#flushCallbacks();
    | MouseEvent(me) => Revery_UI.Mouse.dispatch(mouseCursor, me, rootNode)
    | KeyboardEvent(ke) => Revery_UI.Keyboard.dispatch(ke)
    | SetSyntax(v) =>
      print_endline("Got SetSyntax event!");
      switch (v) {
      | ML => Repl.SyntaxControl.ml()
      | RE => Repl.SyntaxControl.re()
      };
      switch (latestCode^, v) {
      | (Some(code), Protocol.Syntax.ML) =>
        let result =
          code |> RefmtJsApi.parseRE |> RefmtJsApi.printML |> Js.string;
        sendMessage(Protocol.ToRenderer.SyntaxChanged(result));
      | (Some(code), Protocol.Syntax.RE) =>
        let result =
          code |> RefmtJsApi.parseML |> RefmtJsApi.printRE |> Js.string;
        sendMessage(Protocol.ToRenderer.SyntaxChanged(result));
      | (None, _) => print_endline("No source code to convert")
      };
    }
  );

  log("Initialized");
  sendMessage(Protocol.ToRenderer.Ready);

  () => {
    Revery.Tick.pump();
    Revery.UI.AnimationTicker.tick();

    if (Revery.UI.Animated.anyActiveAnimations() || dirty^) {
      dirty := false;
      render();
    };
  };
};
