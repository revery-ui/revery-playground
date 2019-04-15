open Revery;
open Revery.UI;

open PlaygroundLib;
open PlaygroundLib.Types;

open Js_of_ocaml;

/* let log = v => print_endline("[Worker] " ++ v); */
let log = _ => ();

let renderFunction =
  ref(() =>
    <View
      style=Style.[backgroundColor(Colors.red), width(100), height(100)]
    />
  );

let container = ref(Container.create(NodeProxies.rootNode));
let rootNode = NodeProxies.rootNode;

let sendMessage = msg => {
  Worker.post_message(msg);
};

let render = () => {
  log(
    "before render function - childCount: "
    ++ string_of_int(List.length(rootNode#getChildren())),
  );
  container := Container.update(container^, renderFunction^());
  log(
    "Set render function - childCount: "
    ++ string_of_int(List.length(rootNode#getChildren())),
  );

  log("Trying to post...");
  let updatesToSend = NodeProxies.getUpdates();
  sendMessage(Protocol.ToRenderer.Updates(updatesToSend));
  log("Posted!" ++ string_of_int(List.length(updatesToSend)));
  NodeProxies.clearUpdates();
};

let dirty = ref(true);

let onStale = () => {
  log("onStale - re-rendering");
  dirty := true;
};

let _ = Revery_Core.Event.subscribe(React.onStale, onStale);

let setRenderFunction = fn => {
  renderFunction := fn;
  dirty := true;
};

let start = exec => {
  let mouseCursor = Revery_UI.Mouse.Cursor.make();

  Worker.set_onmessage((updates: Protocol.ToWorker.t) =>
    switch (updates) {
    | SourceCodeUpdated(v) =>
      log("got source code update");
      sendMessage(Protocol.ToRenderer.Compiling);
      let output = Obj.magic(exec(v));
      sendMessage(Protocol.ToRenderer.Output(output));
      sendMessage(Protocol.ToRenderer.Ready);
    | Measurements(v) =>
      log("applying measurements");

      let f = (measurement: Protocol.ToWorker.nodeMeasurement) => {
        let nodeId = measurement.id;
        log("Applying measurement for node: " ++ string_of_int(nodeId));
        let measurements = measurement.dimensions;

        let node = ProxyNodeFactory.getNodeById(nodeId);
        log("forcing measurement");
        node#forceMeasurements(measurements);
        log("forcing measurement done");
      };

      List.iter(f, v);
      rootNode#recalculate();
      rootNode#flushCallbacks();

      log("measurements applied");
    | MouseEvent(me) => Revery_UI.Mouse.dispatch(mouseCursor, me, rootNode)
    | KeyboardEvent(ke) => Revery_UI.Keyboard.dispatch(ke)
    }
  );

  log("Initialized");
  sendMessage(Protocol.ToRenderer.Ready);

  () => {
    Revery.Tick.pump();
    Revery.UI.AnimationTicker.tick();

    if (Revery.UI.Animated.anyActiveAnimations() || dirty^) {
      render();
      dirty := false;
    };
  };
};
