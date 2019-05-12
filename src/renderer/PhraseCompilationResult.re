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

  let content = switch(block) {
  | BlockSuccess({ msg, _ }) => msg
  | _ => ""
  };

  let contentJs = Js.string(content ++ "\n");

  %js
  {
    val startLineNumber = startLine + 1;
    val endLineNumber = endLine + 1;
    val result = s;
	val content = contentJs;
  };
};
