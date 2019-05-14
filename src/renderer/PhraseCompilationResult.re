open Js_of_ocaml;

let compiling = Js.string("compiling");
let compiled = Js.string("compiled");
let error = Js.string("error");

let toCompiledJs = (evalId, startLine, endLine) => {
  %js
  {
    val evalId = evalId;
    val startLineNumber = startLine + 1;
    val endLineNumber = endLine + 1;
    val result = compiled
  };
};

let toJs = (evalId, startLine, endLine, block: Core.Evaluate.blockContent) => {
  let s =
    switch (block) {
    | BlockStart => compiling
    | BlockSuccess(_) => compiled
    | BlockError(_) => error
    };

  %js
  {
    val evalId = evalId;
    val startLineNumber = startLine + 1;
    val endLineNumber = endLine + 1;
    val result = s
  };
};
