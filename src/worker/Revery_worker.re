open Js_of_ocaml;
open Js_of_ocaml_toplevel;

open Playground;
open Worker;

let start = () => {
  Playground.reasonSyntax();
  JsooTop.initialize();

  let render = Backend.start(execute);

  /* Return a callback to the callee to 'tick' the app */
  let f = _ => {
    render();
  };
  Js.Unsafe.callback(f);
};

let () = Js.export_all([%js {val startWorker = start}]);
