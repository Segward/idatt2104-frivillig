const WS_URL = `wss://${location.host}`;

const counterValEl = document.getElementById("counter-val");
const incBtn = document.getElementById("inc");
const decBtn = document.getElementById("dec");
const listItemsEl = document.getElementById("list-items");
const listInput = document.getElementById("list-input");
const listAddBtn = document.getElementById("list-add-btn");
const textArea = document.getElementById("text-area");
const statusEl = document.getElementById("status");

let clientId = null;
let ws = null;

// --- PN counter (state-based: send full state, merge by max-per-id) ---

const counterState = { increments: {}, decrements: {} };

function counterRecompute() {
  let total = 0;
  for (const v of Object.values(counterState.increments)) total += v;
  for (const v of Object.values(counterState.decrements)) total -= v;
  counterValEl.textContent = total;
}

function counterMerge(remote) {
  const join = (target, incoming) => {
    for (const [k, v] of Object.entries(incoming || {})) {
      target[k] = Math.max(target[k] || 0, v);
    }
  };
  join(counterState.increments, remote.increments);
  join(counterState.decrements, remote.decrements);
}

function counterSend() {
  if (!ws || ws.readyState !== WebSocket.OPEN) return;
  ws.send(JSON.stringify({
    type: "counter_state",
    increments: counterState.increments,
    decrements: counterState.decrements,
  }));
}

function counterBumpInc() {
  if (!clientId) return;
  counterState.increments[clientId] = (counterState.increments[clientId] || 0) + 1;
  counterRecompute();
  counterSend();
}

function counterBumpDec() {
  if (!clientId) return;
  counterState.decrements[clientId] = (counterState.decrements[clientId] || 0) + 1;
  counterRecompute();
  counterSend();
}

incBtn.addEventListener("click", counterBumpInc);
decBtn.addEventListener("click", counterBumpDec);

// --- shared RGA helpers (used by both list and text) ---

function rgaInsertNode(state, node) {
  if (state.nodes.has(node.element_id)) return;
  state.nodes.set(node.element_id, { ...node });
  const siblings = state.children.get(node.previous_id) || [];
  if (!siblings.includes(node.element_id)) siblings.push(node.element_id);
  siblings.sort();
  state.children.set(node.previous_id, siblings);
  if (!state.children.has(node.element_id)) state.children.set(node.element_id, []);
}

function rgaMerge(state, remoteNodes) {
  // Multi-pass: insert when previous_id is present; OR tombstones for known nodes.
  let pending = remoteNodes.slice();
  let madeProgress = true;
  while (madeProgress) {
    madeProgress = false;
    const stillPending = [];
    for (const incoming of pending) {
      const existing = state.nodes.get(incoming.element_id);
      if (existing) {
        if (incoming.deleted && !existing.deleted) existing.deleted = true;
        madeProgress = true;
        continue;
      }
      if (incoming.previous_id && !state.nodes.has(incoming.previous_id)) {
        stillPending.push(incoming);
        continue;
      }
      rgaInsertNode(state, incoming);
      madeProgress = true;
    }
    pending = stillPending;
    if (pending.length === 0) break;
  }
}

function rgaStateNodes(state) {
  const out = [];
  for (const node of state.nodes.values()) {
    out.push({
      element_id: node.element_id,
      previous_id: node.previous_id,
      value: node.value,
      deleted: node.deleted,
    });
  }
  return out;
}

function rgaWalk(state, visit) {
  const recur = (parent) => {
    const kids = state.children.get(parent) || [];
    for (const childId of kids) {
      const n = state.nodes.get(childId);
      visit(n);
      recur(childId);
    }
  };
  recur("");
}

// --- RGA list ---

const listState = {
  nodes: new Map(),
  children: new Map([["", []]]),
  sequence: 0,
};

function listNextId() {
  listState.sequence += 1;
  return `${clientId}:${String(listState.sequence).padStart(20, "0")}`;
}

function listVisibleNodes() {
  const out = [];
  rgaWalk(listState, (n) => { if (!n.deleted) out.push(n); });
  return out;
}

function listRender() {
  const items = listVisibleNodes();
  listItemsEl.innerHTML = "";
  for (const item of items) {
    const li = document.createElement("li");
    const span = document.createElement("span");
    span.textContent = item.value;
    const btn = document.createElement("button");
    btn.textContent = "×";
    btn.addEventListener("click", () => listDelete(item.element_id));
    li.append(span, btn);
    listItemsEl.append(li);
  }
}

function listSendState() {
  if (!ws || ws.readyState !== WebSocket.OPEN) return;
  ws.send(JSON.stringify({ type: "list_state", nodes: rgaStateNodes(listState) }));
}

function listAddAtEnd(value) {
  if (!clientId || !value) return;
  const visible = listVisibleNodes();
  const previousId = visible.length ? visible[visible.length - 1].element_id : "";
  const id = listNextId();
  rgaInsertNode(listState, { element_id: id, previous_id: previousId, value, deleted: false });
  listRender();
  listSendState();
}

function listDelete(elementId) {
  if (!clientId) return;
  const node = listState.nodes.get(elementId);
  if (!node || node.deleted) return;
  node.deleted = true;
  listRender();
  listSendState();
}

listAddBtn.addEventListener("click", () => {
  const v = listInput.value.trim();
  if (!v) return;
  listAddAtEnd(v);
  listInput.value = "";
  listInput.focus();
});
listInput.addEventListener("keydown", (e) => {
  if (e.key === "Enter") listAddBtn.click();
});

// --- RGA text ---

const textState = {
  nodes: new Map(),
  children: new Map([["", []]]),
  sequence: 0,
};
let textSuppressInput = false;

function textNextId() {
  textState.sequence += 1;
  return `${clientId}:t:${String(textState.sequence).padStart(20, "0")}`;
}

function textRenderFlat() {
  const chars = [];
  const ids = [];
  rgaWalk(textState, (n) => {
    if (!n.deleted) { chars.push(n.value); ids.push(n.element_id); }
  });
  return { value: chars.join(""), ids };
}

function textSendState() {
  if (!ws || ws.readyState !== WebSocket.OPEN) return;
  ws.send(JSON.stringify({ type: "text_state", nodes: rgaStateNodes(textState) }));
}

function textApplyEdit(oldStr, newStr, oldIds) {
  let p = 0;
  while (p < oldStr.length && p < newStr.length && oldStr[p] === newStr[p]) p++;
  let so = oldStr.length;
  let sn = newStr.length;
  while (so > p && sn > p && oldStr[so - 1] === newStr[sn - 1]) { so--; sn--; }

  for (let i = p; i < so; i++) {
    const node = textState.nodes.get(oldIds[i]);
    if (node) node.deleted = true;
  }

  let previousId = p > 0 ? oldIds[p - 1] : "";
  for (let i = p; i < sn; i++) {
    const id = textNextId();
    rgaInsertNode(textState, {
      element_id: id,
      previous_id: previousId,
      value: newStr[i],
      deleted: false,
    });
    previousId = id;
  }
}

function textRender() {
  const { value } = textRenderFlat();
  if (textArea.value === value) return;
  const start = textArea.selectionStart;
  const end = textArea.selectionEnd;
  textSuppressInput = true;
  textArea.value = value;
  textSuppressInput = false;
  const cap = value.length;
  textArea.setSelectionRange(Math.min(start, cap), Math.min(end, cap));
}

textArea.addEventListener("input", () => {
  if (textSuppressInput || !clientId) return;
  const newStr = textArea.value;
  const { value: rendered, ids } = textRenderFlat();
  textApplyEdit(rendered, newStr, ids);
  textSendState();
});

// --- transport ---

function connect() {
  ws = new WebSocket(WS_URL);

  ws.addEventListener("open", () => {
    statusEl.textContent = "connected, waiting for auth…";
  });

  ws.addEventListener("message", (evt) => {
    let msg;
    try { msg = JSON.parse(evt.data); } catch { return; }
    if (!msg || typeof msg !== "object") return;

    if (msg.type === "auth") {
      clientId = msg.id;
      statusEl.textContent = `connected as ${clientId}`;
      return;
    }
    if (msg.type === "counter_state") {
      counterMerge(msg);
      counterRecompute();
      return;
    }
    if (msg.type === "list_state") {
      rgaMerge(listState, msg.nodes || []);
      listRender();
      return;
    }
    if (msg.type === "text_state") {
      rgaMerge(textState, msg.nodes || []);
      textRender();
      return;
    }
  });

  ws.addEventListener("close", () => {
    statusEl.textContent = "disconnected";
  });

  ws.addEventListener("error", () => {
    statusEl.textContent = "connection error";
  });
}

connect();
