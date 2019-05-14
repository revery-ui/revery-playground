open Js_of_ocaml;
open Js_of_ocaml_toplevel;

open Revery;
open Revery.UI;

open PlaygroundLib;
open PlaygroundLib.Types;

PlaygroundLib.Worker.handler := Some(Backend.setRenderFunction);

let postfix = () =>
  switch (Repl.SyntaxControl.current()) {
  | Core.Syntax.RE => "\nPlaygroundLib.Worker.setRenderFunction(render);"
  | Core.Syntax.ML => "\n;;PlaygroundLib.Worker.setRenderFunction render;;"
  };

module Stdout = {
  type capture = unit;
  let start = () => ();
  let stop = () => "";
};

let previous: ref(Repl.Evaluate.t) = ref(Repl.Evaluate.empty);

let execute = (~send, ~complete, code) => {
  let code = code ++ postfix();

  previous :=
    Repl.Evaluate.eval(
      ~previous=previous^,
      ~send,
      ~complete,
      ~readStdout=(module Stdout),
      code,
    );

  %js
  {val result = ""; val stderr = ""; val stdout = ""};
};
