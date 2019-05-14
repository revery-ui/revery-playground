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

let phraseTerminator = () =>
  switch (Repl.SyntaxControl.current()) {
  | Core.Syntax.RE => ";"
  | Core.Syntax.ML => ";;"
  };

module Stdout = {
  type capture = unit;
  let start = () => ();
  let stop = () => "";
};

let previous: ref(Repl.Evaluate.t) = ref(Repl.Evaluate.empty);

let complete = code => {
  /* Set up runtime environment - evaluate as much as we can from cached state */

  let code = Js.to_string(code);

  let send = _ => ();
  let complete = _ => ();

  let _ =
    Repl.Evaluate.eval(
      ~previous=previous^,
      ~send,
      ~complete,
      ~readStdout=(module Stdout),
      code,
    );

  let (startPos, completions) =
    VendoredUTop.UTop_complete.complete(
      ~syntax=Normal,
      ~phrase_terminator=phraseTerminator(),
      ~input=code,
    );
  completions
  |> List.map(v => v |> fst |> Js.string)
  |> Array.of_list
  |> Js.array;
};

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
