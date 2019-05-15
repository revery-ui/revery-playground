const getCachedCode = () => {
  const defaultItem = {
    syntax: "re",
    exampleId: "Hello",
    code: null
  };
  let cachedItem = localStorage.getItem("__revery_playground_cache");
  try {
    cachedItem = !!cachedItem ? JSON.parse(cachedItem) : defaultItem;
  } catch (ex) {
    console.warn(ex);
    cachedItem = defaultItem;
  }

  return cachedItem;
};

const setCachedCode = (exampleId, syntax, code) => {
  localStorage.setItem(
    "__revery_playground_cache",
    JSON.stringify({
      exampleId: exampleId,
      code: code,
      syntax: syntax
    })
  );
};

let createCodeLensProvider = () => {
  let emitter = new monaco.Emitter();

  let _latestCodeLenses = [];

  let update = items => {
    _latestCodeLenses = items
      .map(item => {
        let title = item.command.title;
        return {
          ...item,
          command: {
            title: title
          }
        };
      })
      .filter(item => !!item.command.title);

    emitter.fire(_latestCodeLenses);
  };

  let provider = {
    provideCodeLenses: (model, token) => _latestCodeLenses,
    resolveCodeLens: (model, codeLens, token) => codeLens,
    onDidChange: emitter.event,
    update: update
  };

  return provider;
};

let createCompletionProvider = requestCompletions => {
  let id = 0;
  let pendingCallbacks = {};

  const provideCompletionItems = (model, position) => {
    var textUntilPosition = model.getValueInRange({
      startLineNumber: 1,
      endLineNumber: position.lineNumber,
      startColumn: 1,
      endColumn: position.column
    });

    let newId = id++;
    requestCompletions(newId, textUntilPosition);

    return new Promise(resolve => {
      pendingCallbacks[newId] = resolve;
    });
  };

  let resolver = (id, completions) => {
    if (pendingCallbacks[id]) {
      let resolve = pendingCallbacks[id];
      let result = completions.map(result => ({
        label: result,
        documentation: "",
        kind: monaco.languages.CompletionItemKind.Function,
        insertText: result
      }));

      resolve({ suggestions: result });
      pendingCallbacks[id] = null;
    }
  };

  let completionProvider = {
    triggerCharacters: ["."],
    provideCompletionItems
  };

  return [completionProvider, resolver];
};

let codeLensProvider = createCodeLensProvider();
monaco.languages.registerCodeLensProvider("rust", codeLensProvider);

const startEditor = onComplete => {
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
    fontFamily: null,
    fontLigatures: false,
    codeLens: false
  });
  let iframe = document.getElementById("example_frame");

  let sendMessage = (type, payload) =>
    iframe.contentWindow.postMessage({ type: type, payload: payload }, "*");

  document.getElementById("example-picker").addEventListener("change", v => {
    let example = document.getElementById("example-picker").value;
    fetchLatestSources(example);
    setCachedCode(
      example,
      document.getElementById("syntax-picker").value,
      null
    );
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
      setCachedCode(
        document.getElementById("example-picker").value,
        document.getElementById("syntax-picker").value,
        allLines
      );
    },
    1000,
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
  let lastCodeLenses = [];

  let clearCompileStatus = () => {
    editor.deltaDecorations(lastDecorations, []);
    lastDecorations = [];
    rowToDecoration = {};
    codeLensProvider.update(lastCodeLenses);
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

    let latestLine = item.endLineNumber;

    let content = item.content.trim();
    content = content.split("\n")[0];
    content = content.split("|")[0];

    if (content && content.indexOf("- : unit = ()") < 0) {
      let newCodeLens = {
        range: new monaco.Range(item.startLineNumber, 1, item.endLineNumber, 1),
        id: lastCodeLenses.length.toString(),
        command: {
          title: content
        },
        __evalId: item.evalId
      };

      lastCodeLenses.push(newCodeLens);
    }
    lastCodeLenses = lastCodeLenses.filter(v => {
      return (
        v.__evalId === item.evalId ||
        v.range.startLineNumber >= item.endLineNumber
      );
    });
    codeLensProvider.update(lastCodeLenses);

    rowToDecoration[item.startLineNumber] = newDecoration;
    lastDecorations.push(newDecoration);
  };

  let requestCompletions = (id, v) =>
    sendMessage("editor.requestCompletions", { id: id, text: v });
  let [completionProvider, completionResolver] = createCompletionProvider(
    requestCompletions
  );
  monaco.languages.registerCompletionItemProvider("rust", completionProvider);
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
      } else if (data && data.type == "loaded") {
        onComplete();
      } else if (data && data.type == "editor.completions") {
        let id = data.payload.id;
        let completions = data.payload.completions;
        completionResolver(id, completions);
      }
    },
    false
  );

  let cacheInfo = getCachedCode();
  document.getElementById("example-picker").value = cacheInfo.exampleId;

  window.__revery_editor_set = t => {
    editor.setValue(t);
    editor.revealLine(1);
  };

  if (cacheInfo.syntax) {
    sendMessage("editor.setSyntax", cacheInfo.syntax);
    document.getElementById("syntax-picker").value = cacheInfo.syntax;
  }

  if (!cacheInfo.code) {
    fetchLatestSources("Hello");
  } else {
    window.__revery_latest_sources = cacheInfo.code;
    window.__revery_editor_set(cacheInfo.code);
  }

  document.getElementById("toggle-codelens").addEventListener("click", chk => {
    const isChecked = chk.target.checked;
    editor.updateOptions({ codeLens: isChecked });
  });

  document
    .getElementById("toggle-ligature")
    .addEventListener("click", function toggleLigature(chk) {
      editor.updateOptions({
        fontFamily: chk.target.checked ? "'Fira Code', monospace" : null,
        fontLigatures: chk.target.checked
      });
    });
};
