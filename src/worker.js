
/*
 * Create a '_time' instance:
 * https://github.com/bryphe/reason-glfw/blob/793be75dc868bf3834c03811501f4e1b0c4f8943/src/glfw_stubs.js#L3
 *
 * glfwInit is not called in the worker, so that particular block is not executed
 */
var _time = {
    start: Date.now(),
    offset: 0,
};

importScripts("./gl-matrix-min.js");
importScripts("./Playground.js");

let tickFunction = startWorker();

setInterval(() => {
 console.log("worker tick");   
    let t = 1.1;
    tickFunction(t);
}, 10);

// execute("print_endline(\"hello!\");")

// execute2("open Revery;\n open Revery.UI;\n let render = () => <View style=Style.[width(200), height(200), backgroundColor(Colors.red)] />;\n");
