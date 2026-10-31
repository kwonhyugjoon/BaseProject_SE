"use strict";

const states = {
    standing: "고정 배치", idle: "휴식", walking: "이동", returning: "복귀",
    talking: "대화", watching_player: "플레이어 바라보기", blocked: "길 막힘",
    distant: "원거리 대기", waiting: "화면 전환 대기", paused: "일시정지",
    inactive_area: "다른 지역 · 갱신 중단", disabled: "Actor 비활성"
};
const mutedStates = new Set(["standing", "idle", "distant", "waiting", "paused", "inactive_area", "disabled"]);
const byId = (id) => document.getElementById(id);
let latest = null;

function element(tag, text, className = "") {
    const node = document.createElement(tag);
    node.textContent = text;
    node.className = className;
    return node;
}

function connection(kind, label, message = "") {
    byId("connection").className = `badge ${kind}`;
    byId("connection").textContent = label;
    byId("notice").textContent = message;
    byId("notice").hidden = !message;
    document.body.classList.toggle("disconnected", kind !== "live");
}

function villageField(list, name, value) {
    list.append(element("dt", name), element("dd", value ?? "미구현", value == null ? "unknown" : ""));
}

function renderVillages(snapshot) {
    const fragment = document.createDocumentFragment();
    const choices = {listen: "낯선 목소리 듣기", close: "종 잠재우기"};
    for (const village of snapshot.villages) {
        const card = element("article", "", "village");
        const title = element("div", "", "village-title");
        title.append(element("h3", village.name), element("span", `${village.population}명`, "population"));
        const fields = element("dl", "");
        villageField(fields, "분위기", village.mood);
        villageField(fields, "성향", village.disposition);
        villageField(fields, "소문", village.rumors == null ? null : (village.rumors.join(" / ") || "등록된 소문 없음"));
        villageField(fields, "호수 선택", choices[village.lakeChoice] ?? "아직 선택하지 않음");
        card.append(title, fields);
        fragment.append(card);
    }
    if (!snapshot.villages.length) fragment.append(element("p", "등록된 마을 없음", "empty"));
    byId("villages").replaceChildren(fragment);
}

function renderMobs(snapshot) {
    const fragment = document.createDocumentFragment();
    for (const mob of snapshot.hunting.types) {
        const row = element("tr", "");
        for (const value of [mob.name, mob.total, mob.alive, mob.dead]) {
            row.append(element("td", snapshot.hunting.loaded || typeof value === "string" ? value : "—"));
        }
        fragment.append(row);
    }
    byId("mobs").replaceChildren(fragment);
    byId("hunting-state").textContent = !snapshot.hunting.loaded
        ? "미방문 · 첫 사냥터에 진입하면 몹이 생성됩니다."
        : snapshot.scene !== "tutorial"
            ? (snapshot.paused ? "탐험 지역 · 일시정지" : "탐험 지역 · 갱신 중")
            : "다른 지역 · 마지막 사냥터 상태 유지";
}

function renderNpcs() {
    if (!latest) return;
    const query = byId("search").value.trim().toLocaleLowerCase();
    const state = byId("state-filter").value;
    const npcs = latest.npcs.filter((npc) => (!state || npc.state === state)
        && `${npc.name} ${npc.id} ${npc.routine}`.toLocaleLowerCase().includes(query));
    const fragment = document.createDocumentFragment();
    for (const npc of npcs) {
        const row = element("tr", "");
        const name = element("td", npc.name);
        name.append(element("small", npc.id));
        const status = element("td", "");
        const tone = npc.state === "blocked" ? "blocked" : mutedStates.has(npc.state) ? "muted" : "";
        status.append(element("span", states[npc.state] ?? npc.state, `state ${tone}`));
        row.append(name, element("td", npc.routine), status,
            element("td", `${npc.position.x.toFixed(2)}, ${npc.position.y.toFixed(2)}`),
            element("td", ["동", "남", "서", "북"][npc.facing] ?? "—"),
            element("td", `${npc.visible ? "표시" : "숨김"} / ${npc.enabled ? "활성" : "비활성"}`));
        fragment.append(row);
    }
    if (!npcs.length) {
        const row = element("tr", "");
        const cell = element("td", "검색 조건에 맞는 NPC가 없습니다.", "empty");
        cell.colSpan = 6;
        row.append(cell);
        fragment.append(row);
    }
    byId("npcs").replaceChildren(fragment);
    byId("npc-visible").textContent = `${npcs.length} / ${latest.npcs.length}`;
}

function render(snapshot) {
    latest = snapshot;
    byId("npc-count").textContent = snapshot.counts.npcs;
    byId("village-count").textContent = snapshot.counts.villages;
    byId("alive-count").textContent = snapshot.hunting.loaded ? snapshot.counts.aliveMobs : "—";
    byId("dead-count").textContent = snapshot.hunting.loaded ? snapshot.counts.deadMobs : "—";
    byId("mob-total").textContent = snapshot.hunting.loaded ? `전체 배치 ${snapshot.counts.mobs}개` : "사냥터에 진입하면 집계";
    const sceneName = {tutorial: "갈대여울", first_level: "옛 드워프 교역로", basin: "솔바람 분지",
        echo_hall: "메아리 숲길", stone_ridge: "돌등성이 전초지", mist_shore: "안개 호숫가"};
    byId("scene").textContent = `${sceneName[snapshot.scene] || snapshot.scene}${snapshot.paused ? " / 일시정지" : ""}`;
    byId("updated").textContent = `마지막 상태: ${new Date(snapshot.sampledAt).toLocaleTimeString("ko-KR")}`;
    renderVillages(snapshot);
    renderMobs(snapshot);
    renderNpcs();
}

async function poll() {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 2500);
    try {
        const response = await fetch("/api/state", {cache: "no-store", signal: controller.signal});
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const snapshot = await response.json();
        if (snapshot.schemaVersion !== 1 || !Array.isArray(snapshot.npcs)
            || !Array.isArray(snapshot.villages) || !Array.isArray(snapshot.hunting?.types)) {
            throw new Error("지원하지 않는 JSON 상태 형식");
        }
        render(snapshot);
        if (Date.now() - snapshot.sampledAt > 4000) {
            connection("stale", "갱신 지연", "새 게임 상태가 도착하지 않고 있습니다. 아래는 마지막으로 받은 상태입니다.");
        } else {
            connection("live", "연결됨 · 1초 갱신");
        }
    } catch (error) {
        connection("offline", "연결 끊김", latest
            ? "게임 연결이 끊겼습니다. 아래는 마지막 수신값이며, 게임을 다시 실행하면 자동으로 재연결합니다."
            : "게임을 실행해 주세요. 연결되면 상태가 표시됩니다. (127.0.0.1:8765)");
    } finally {
        clearTimeout(timeout);
        setTimeout(poll, 1000);
    }
}

for (const [value, label] of Object.entries(states)) {
    const option = element("option", label);
    option.value = value;
    byId("state-filter").append(option);
}
byId("search").addEventListener("input", renderNpcs);
byId("state-filter").addEventListener("change", renderNpcs);
poll();
