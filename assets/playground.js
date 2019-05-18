let clearErrors = () => {
  errors = [];
  let errorContainer = document.getElementById("error-container");
  errorContainer.classList.remove("errors");
  sendParentMessage({
    type: "errors.clear"
  });
};

let totalLines = 1;

let sendParentMessage = (v) => {
	window.parent.postMessage(v, "*");
};

let onCompiling = evalId => {
  clearErrors();
  var element = document.getElementById("loading-container");
  element.classList.add("loading");

  sendParentMessage({ type: "compileStatus.clear" });
};

let onInitialized = () => {
  sendParentMessage({ type: "loaded" });
};

let onRender = () => {
  document.getElementById("loading-container").classList.remove("loading");
};

let errors = [];

let updateErrorUI = errors => {
  let errorContainer = document.getElementById("error-container");
  errorContainer.innerHTML = "";

  let errorElements = errors.map(e => {
    let span = document.createElement("p");
    span.textContent =
      "Error " +
      e.startLineNumber +
      "," +
      e.startColumn +
      " - " +
      e.endLineNumber +
      "," +
      e.endColumn +
      ":" +
      e.message;
    return span;
  });
  errorElements.forEach(e => errorContainer.appendChild(e));

  if (errorElements.length >= 1) {
    errorContainer.classList.add("errors");
  }
};

let onError = err => {
  errors.push(err);
  updateErrorUI(errors);
  sendParentMessage({
    type: "errors.append",
    payload: [err]
  });
};

let onOutput = e => {};

let updateProgress = line => {
  let currentCompilationLine = line;
  let progress = (line / totalLines) * 100;

  if (progress > 99) {
    progress = 0;
  }
  document.getElementById("progress-bar").style.width = progress + "%";
};

let onCompilationResult = r => {
  sendParentMessage({ type: "compileStatus.append", payload: r });

  if (r.result == "compiled") {
    updateProgress(r.endLineNumber);
  } else if (r.result == "compiling") {
    updateProgress((r.startLineNumber + r.endLineNumber) / 2);
  }
};

let onSyntaxChanged = newCode => {
  sendParentMessage({
    type: "syntax.change",
    code: newCode
  });
};

let onCompletions = (id, completions) => {
  sendParentMessage({
    type: "editor.completions",
    payload: {
      id,
      completions
    }
  });
};

let [updateCode, setSyntax, requestCompletions] = startRenderer(
  onInitialized,
  onCompiling,
  onRender,
  onOutput,
  onSyntaxChanged,
  onError,
  onCompilationResult,
  onCompletions
);

window.addEventListener("message", msg => {
  if (msg.data.type === "editor.update") {
    document.getElementById("progress-bar").style.width = "0%";
    let code = msg.data.payload;
    lines = code.split("\n").length;
    totalLines = lines > 0 ? lines : 1;

    updateCode(code);
  } else if (msg.data.type === "editor.setSyntax") {
    setSyntax(msg.data.payload);
  } else if (msg.data.type === "editor.requestCompletions") {
    let { id, text } = msg.data.payload;
    requestCompletions(id, text);
  }
});
