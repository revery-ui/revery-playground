module Protocol = Protocol;
module Types = Types;

open Revery.UI;

/*
 * This API needs to be exposed in the toplevel (which `Playground.Lib` is included)
 * so that we can call `setRenderFunction` after the user code
 */
module Worker = {
  let handler = ref(None);

  let setRenderFunction = (v: unit => React.syntheticElement) => {
    switch (handler^) {
    | Some(handler) => handler(v)
    | None => print_endline("NO HANDLER")
    };
  };
};
