
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
    let t = 1.1;
    tickFunction(t);
}, 10);
