/**
  NodeProxies

  This module creates a set of worker-side Nodes.

  The challenge here is that the Worker doesn't have access to any graphical context,
  at least until OffscreenCanvas becomes widely supported.

  Therefore, we create a hierarchy of nodes here, that 'proxy' updates
  to the renderer.
*/

open Revery.UI;

open PlaygroundLib;
open PlaygroundLib.Types;

/* Track the updates we need to send to the renderer */
let _pendingUpdates: ref(list(updates)) = ref([]);
let clearUpdates = () => _pendingUpdates := [];
let queueUpdate = (update: updates) => {
  _pendingUpdates := [update, ..._pendingUpdates^];
};

let getUpdates = () => _pendingUpdates^ |> List.rev;


class proxyViewNode (()) = {
  as _this;
  inherit (class viewNode)() as super;
  pub! setStyle = style => {
    queueUpdate(SetStyle(super#getInternalId(), style));
  };
  pub! addChild = child => {
    queueUpdate(AddChild(super#getInternalId(), child#getInternalId()));
    super#addChild(child);
  };
  pub! removeChild = child => {
    queueUpdate(RemoveChild(super#getInternalId(), child#getInternalId()));
    super#removeChild(child);
  };
  initializer {
    queueUpdate(NewNode(super#getInternalId(), View));
  };
};

class proxyTextNode (text) = {
  as _this;
  inherit (class textNode)(text) as super;
  pub! setStyle = style => {
    queueUpdate(SetStyle(super#getInternalId(), style));
  };
  pub! addChild = child => {
    queueUpdate(AddChild(super#getInternalId(), child#getInternalId()));
    super#addChild(child);
  };
  pub! removeChild = child => {
    queueUpdate(RemoveChild(super#getInternalId(), child#getInternalId()));
    super#removeChild(child);
  };
  pub! setText = text => {
    queueUpdate(SetText(super#getInternalId(), text));
  };
  initializer {
    queueUpdate(NewNode(super#getInternalId(), Text));
    queueUpdate(SetText(super#getInternalId(), text));
  };
};

class proxyImageNode (src) = {
  as _this;
  inherit (class imageNode)(src) as super;
  pub! setStyle = style => {
    queueUpdate(SetStyle(super#getInternalId(), style));
  };
  pub! addChild = child => {
    queueUpdate(AddChild(super#getInternalId(), child#getInternalId()));
    super#addChild(child);
  };
  pub! removeChild = child => {
    queueUpdate(RemoveChild(super#getInternalId(), child#getInternalId()));
    super#removeChild(child);
  };
  pub! setSrc = src => {
    queueUpdate(SetImageSrc(super#getInternalId(), src));
    super#setSrc(src);
  };
  initializer {
    queueUpdate(NewNode(super#getInternalId(), Image));
    queueUpdate(SetImageSrc(super#getInternalId(), src));
  };
};

let rootNode = (new proxyViewNode)();
queueUpdate(RootNode(rootNode#getInternalId()));
