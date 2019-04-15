open Revery.UI;
open Revery.UI.Internal.PrimitiveNodeFactory;

open NodeProxies;

let idToNode: Hashtbl.t(int, viewNode) = Hashtbl.create(100);

let registerNode = node => {
  Hashtbl.add(idToNode, node#getInternalId(), Obj.magic(node));
};

registerNode(NodeProxies.rootNode);

let getNodeById = id => {
  switch (Hashtbl.find_opt(idToNode, id)) {
  | Some(v) => v
  | None => failwith("Unable to find node with id: " ++ string_of_int(id))
  };
};

let proxyNodeFactory: nodeFactory = {
  createViewNode: () => {
    let ret = (new proxyViewNode)();
    registerNode(ret);
    ret;
  },
  createTextNode: text => {
    let ret = (new proxyTextNode)(text);
    registerNode(ret);
    ret;
  },
  createImageNode: src => {
    let ret = (new proxyImageNode)(src);
    registerNode(ret);
    ret;
  },
};

Revery.UI.Internal.PrimitiveNodeFactory.set(proxyNodeFactory);
