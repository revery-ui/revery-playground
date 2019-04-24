module Protocol = Protocol;
module Types = Types;

open Revery.UI;

module Worker = {
  let handler = ref(None);

  let setRenderFunction = (v: unit => React.syntheticElement) => {
    switch (handler^) {
    | Some(handler) => handler(v)
    | None => print_endline("NO HANDLER")
    };
  };
};
