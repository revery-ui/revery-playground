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

let execute = code => {
  let code = code ++ postfix();
  let send = r => print_endline("RESULT: " ++ Core.Evaluate.show_result(r));
  let complete = eval =>
    switch (eval) {
    | Core.Evaluate.EvalSuccess => print_endline("complete: success")
    | Core.Evaluate.EvalError => print_endline("complete: error")
    | Core.Evaluate.EvalInterupted => print_endline("complete: interrupted")
    };

  Repl.Evaluate.eval(~send, ~complete, ~readStdout=(module Stdout), code);

  %js
  {val result = ""; val stderr = ""; val stdout = ""};
};
