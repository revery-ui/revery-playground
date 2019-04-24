open Js_of_ocaml;
open Js_of_ocaml_toplevel;

open Revery;
open Revery.UI;

open PlaygroundLib;
open PlaygroundLib.Types;

let stderr_buffer = Buffer.create(100);
let stdout_buffer = Buffer.create(100);

/* Sys_js.set_channel_flusher(stdout, Buffer.add_string(stdout_buffer)); */
Sys_js.set_channel_flusher(stderr, Buffer.add_string(stderr_buffer));

let execute: Js.t(Js.js_string) => Js.t(Js.js_string) =
  code => {
    let code = Js.to_string(code);
    let buffer = Buffer.create(100);
    let formatter = Format.formatter_of_buffer(buffer);
    JsooTop.execute(true, formatter, code);
    let result = Buffer.contents(buffer);
    Js.string(result);
  };

PlaygroundLib.Worker.handler := Some(Backend.setRenderFunction);

let postfix = () =>
  switch (Syntax.current()) {
  | Protocol.Syntax.RE => "\nPlaygroundLib.Worker.setRenderFunction(render);"
  | Protocol.Syntax.ML => "\n;;PlaygroundLib.Worker.setRenderFunction render;;"
  };

let execute2 = code => {
  let code = code ++ postfix();
  let buffer = Buffer.create(100);
  let formatter = Format.formatter_of_buffer(buffer);
  JsooTop.execute(true, formatter, code);

  let result = Buffer.contents(buffer);
  let stderr_result = Buffer.contents(stderr_buffer);
  let stdout_result = Buffer.contents(stdout_buffer);

  Buffer.clear(stderr_buffer);
  Buffer.clear(stdout_buffer);

  %js
  {
    val result = Js.string(result);
    val stderr = Js.string(stderr_result);
    val stdout = Js.string(stdout_result)
  };
};
