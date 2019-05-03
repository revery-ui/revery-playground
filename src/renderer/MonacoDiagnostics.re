open Js_of_ocaml;

type t = {
  startLineNumber: int,
  endLineNumber: int,
  startColumn: int,
  endColumn: int,
  message: string,
};

let errorToDiagnostic = (error: Core.Evaluate.error) => {
  let (startLineNumber, endLineNumber, startColumn, endColumn) =
    switch (error.errLoc) {
    | None => (1, 1, 0, 0)
    | Some(v) =>
      Core.Loc.(
        v.loc_start.line + 1,
        v.loc_end.line + 1,
        v.loc_start.col + 1,
        v.loc_end.col + 2,
      )
    };

  let message = error.errMsg;
  let ret: t = {
    startLineNumber,
    endLineNumber,
    startColumn,
    endColumn,
    message,
  };
  ret;
};

let ofBlockContent: Core.Evaluate.blockContent => option(t) =
  (r: Core.Evaluate.blockContent) => {
    switch (r) {
    | BlockSuccess(_) => None
    | BlockError(v) =>
      let result: t = errorToDiagnostic(v.error);
      Some(result);
    };
  };

/*
 * Strange issue with JSOO when converting the `v.message` string:
 * - If I use Js.string(v.message), I get "[object Object]"
 * - If I use just v.message, I get the raw OCaml object
 * - Concatenating something to it gives a correct message.... not sure why!
 */
let toJs = v => [%js
  {
    val startLineNumber = v.startLineNumber;
    val endLineNumber = v.endLineNumber;
    val startColumn = v.startColumn;
    val endColumn = v.endColumn;
    val message = Js.string(v.message ++ ".")
  }
];
