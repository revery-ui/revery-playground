// Create a setter that will be overridden once an editor is available
window.__revery_latest_sources = "Loading...";
window.__revery_editor_set = t => window.__revery_latest_sources;

let fetchLatestSources = sourceFile => {
  let ext = "." + document.getElementById("syntax-picker").value;
  fetch("sources/" + sourceFile + ext).then(response => {
    response.text().then(t => {
      window.__revery_latest_sources = t;
      window.__revery_editor_set(t);
    });
  });
};

var editor = monaco.editor.create(document.getElementById("code"), {
  value: window.__revery_latest_sources,
  language: "rust",
  theme: "vs-dark",
  readOnly: false,
  fontFamily: "'Fira Code', monospace",
  fontLigatures: false
});
let iframe = document.getElementById("example_frame");

let sendMessage = (type, payload) =>
  iframe.contentWindow.postMessage({ type: type, payload: payload }, "*");

document.getElementById("example-picker").addEventListener("change", v => {
  fetchLatestSources(document.getElementById("example-picker").value);
});

document.getElementById("syntax-picker").addEventListener("change", v => {
  sendMessage(
    "editor.setSyntax",
    document.getElementById("syntax-picker").value
  );
});

// Credit David Walsh (https://davidwalsh.name/javascript-debounce-function)

// Returns a function, that, as long as it continues to be invoked, will not
// be triggered. The function will be called after it stops being called for
// N milliseconds. If `immediate` is passed, trigger the function on the
// leading edge, instead of the trailing.
function debounce(func, wait, immediate) {
  var timeout;

  // This is the function that is actually executed when
  // the DOM event is triggered.
  return function executedFunction() {
    // Store the context of this and any
    // parameters passed to executedFunction
    var context = this;
    var args = arguments;

    // The function to be called after
    // the debounce time has elapsed
    var later = function() {
      // null timeout to indicate the debounce ended
      timeout = null;

      // Call function now if you did not on the leading end
      if (!immediate) func.apply(context, args);
    };

    // Determine if you should call the function
    // on the leading or trail end
    var callNow = immediate && !timeout;

    // This will reset the waiting every function execution.
    // This is the step that prevents the function from
    // being executed because it will never reach the
    // inside of the previous setTimeout
    clearTimeout(timeout);

    // Restart the debounce waiting period.
    // setTimeout returns a truthy value (it differs in web vs node)
    timeout = setTimeout(later, wait);

    // Call immediately if you're dong a leading
    // end execution
    if (callNow) func.apply(context, args);
  };
}

let debouncedUpdate = debounce(
  () => {
    console.log("[index] Sending code update");
    let allLines = editor
      .getModel()
      .getLinesContent()
      .join("\n");
    sendMessage("editor.update", allLines);
  },
  250,
  false
);

editor.onDidChangeModelContent(e => {
  debouncedUpdate();
});

let markers = [];
let clearErrors = () => {
  markers = [];
  monaco.editor.setModelMarkers(editor.getModel(), "revery", markers);
};

let appendErrors = errors => {
  markers = markers.concat(errors);
  monaco.editor.setModelMarkers(editor.getModel(), "revery", markers);
};

let lastDecorations = [];
let rowToDecoration = {};

let clearCompileStatus = () => {
  editor.deltaDecorations(lastDecorations, []);
  lastDecorations = [];
  rowToDecoration = {};
};

let appendCompileStatus = item => {
  let previousDecorations = [];
  if (rowToDecoration[item.startLineNumber]) {
    previousDecorations = [rowToDecoration[item.startLineNumber]];
  }

  let newDecoration = editor.deltaDecorations(previousDecorations, [
    {
      range: new monaco.Range(item.startLineNumber, 1, item.endLineNumber, 1),
      options: {
        isWholeLine: true,
        linesDecorationsClassName: "line-decoration-" + item.result
      }
    }
  ]);

  rowToDecoration[item.startLineNumber] = newDecoration;
  lastDecorations.push(newDecoration);
};

window.addEventListener(
  "message",
  evt => {
    let data = evt.data;
    if (data && data.type == "example.switch") {
      fetchLatestSources(data.payload);
    } else if (data && data.type == "syntax.change") {
      window.__revery_editor_set(data.code);
    } else if (data && data.type == "errors.clear") {
      clearErrors();
    } else if (data && data.type == "errors.append") {
      appendErrors(data.payload);
    } else if (data && data.type == "compileStatus.clear") {
      clearCompileStatus();
    } else if (data && data.type == "compileStatus.append") {
      appendCompileStatus(data.payload);
    }
  },
  false
);

fetchLatestSources("Hello");

window.__revery_editor_set = t => {
  editor.setValue(t);
  editor.revealLine(1);
};

document
  .getElementById("toggle-ligature")
  .addEventListener("click", function toggleLigature(chk) {
    editor.updateOptions({
      fontLigatures: chk.target.checked
    });
  });
