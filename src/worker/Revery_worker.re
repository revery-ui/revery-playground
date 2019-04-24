open Js_of_ocaml;
open Js_of_ocaml_toplevel;

open Playground;
open Worker;

let log = v => print_endline("[Worker] " ++ v);

let start = () => {
  Syntax.reason();
  JsooTop.initialize();

  let render = Backend.start(execute2);
  log("Initialized");

  let f = _ => {
    render();
  };
  Js.Unsafe.callback(f);
};

let () = Js.export_all([%js {val startWorker = start}]);
