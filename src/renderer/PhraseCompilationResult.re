open Js_of_ocaml;

let compiling = Js.string("compiling");
let compiled = Js.string("compiled");
let error = Js.string("error");

let toCompiledJs = (startLine, endLine) => {
  %js
  {
    val startLineNumber = startLine + 1;
    val endLineNumber = endLine + 1;
    val result = compiled
  };
};

let toJs = (startLine, endLine, block: Core.Evaluate.blockContent) => {
  let s =
    switch (block) {
    | BlockStart => compiling
    | BlockSuccess(_) => compiled
    | BlockError(_) => error
    };

  %js
  {
    val startLineNumber = startLine + 1;
    val endLineNumber = endLine + 1;
    val result = s
  };
};
