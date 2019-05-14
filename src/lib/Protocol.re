/*
 * Protocol
 *
 * Types for communication: worker <-> renderer
 */

open Js_of_ocaml;

open Types;

module Syntax = {
  type t =
    | RE
    | ML;
};

module ToWorker = {
  type nodeMeasurement = {
    id: int,
    dimensions: Revery.UI.Dimensions.t,
  };

  type t =
    | SourceCodeUpdated(Js.t(Js.js_string))
    | Measurements(list(nodeMeasurement))
    | KeyboardEvent(Revery_Core.Events.internalKeyboardEvent)
    | MouseEvent(Revery_Core.Events.internalMouseEvents)
    | SetSyntax(Syntax.t);
};

module ToRenderer = {
  type t =
    | Ready
    | Compiling(Core.Evaluate.evalId)
    | Output(Js.t(Js.Unsafe.any))
    | PhraseResult(Core.Evaluate.result)
    | CompilationResult(Core.Evaluate.evalResult)
    | Updates(list(Types.updates))
    | SyntaxChanged(Js.t(Js.js_string));
};
