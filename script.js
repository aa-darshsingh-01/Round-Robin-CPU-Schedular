const COLORS = ["#1f5fbf", "#c2410c", "#0f766e", "#7c3aed", "#b4236a", "#4d7c0f", "#a16207", "#0e7490", "#9f1239", "#475569"];
const $ = s => document.querySelector(s);

/* ---------- Core Round Robin ---------- */
function roundRobin(list, quantum) {
    const procs = list.map((p, i) => ({ ...p, rem: p.burst, order: i }))
        .sort((a, b) => a.arrival - b.arrival || a.order - b.order);
    const queue = [], gantt = [];
    let time = 0, next = 0, done = 0;
    const admit = () => { while (next < procs.length && procs[next].arrival <= time) queue.push(procs[next++]); };
    admit();
    while (done < procs.length) {
        if (!queue.length) {                       // CPU idle until next arrival
            const t = procs[next].arrival;
            gantt.push({ pid: "IDLE", start: time, end: t });
            time = t; admit(); continue;
        }
        const p = queue.shift();
        const run = Math.min(quantum, p.rem);
        gantt.push({ pid: p.name, start: time, end: time + run, idx: p.order });
        time += run; p.rem -= run;
        admit();                                   // new arrivals go before the preempted process
        if (p.rem > 0) queue.push(p);
        else { p.completion = time; p.tat = time - p.arrival; p.wt = p.tat - p.burst; done++; }
    }
    const merged = [];
    for (const s of gantt) {
        const last = merged[merged.length - 1];
        if (last && last.pid === s.pid && last.end === s.start) last.end = s.end;
        else merged.push({ ...s });
    }
    const busy = gantt.filter(s => s.pid !== "IDLE").reduce((a, s) => a + s.end - s.start, 0);
    const total = gantt[gantt.length - 1].end - gantt[0].start;
    return { gantt: merged, procs: procs.sort((a, b) => a.order - b.order), util: busy / total * 100 };
}

/* ---------- Input table ---------- */
function addRow(at = "", bt = "") {
    const tb = $("#input-table tbody");
    const tr = document.createElement("tr");
    tr.innerHTML = `<td class="pname"></td>
    <td><input type="number" min="0" class="at" value="${at}" aria-label="Arrival time"></td>
    <td><input type="number" min="1" class="bt" value="${bt}" aria-label="Burst time"></td>
    <td><button class="del" aria-label="Remove process">Remove</button></td>`;
    tr.querySelector(".del").onclick = () => { tr.remove(); renumber(); };
    tb.appendChild(tr); renumber();
}
function renumber() {
    document.querySelectorAll("#input-table tbody tr").forEach((tr, i) => {
        tr.querySelector(".pname").innerHTML =
            `<span class="chip" style="background:${COLORS[i % COLORS.length]}"></span>P${i + 1}`;
    });
}
function readInput() {
    const rows = [...document.querySelectorAll("#input-table tbody tr")];
    if (!rows.length) throw new Error("Add at least one process.");
    const list = rows.map((tr, i) => {
        const arrival = tr.querySelector(".at").value, burst = tr.querySelector(".bt").value;
        if (arrival === "" || burst === "") throw new Error(`P${i + 1}: fill in both arrival and burst time.`);
        const a = Number(arrival), b = Number(burst);
        if (!Number.isInteger(a) || a < 0) throw new Error(`P${i + 1}: arrival time must be a whole number, 0 or more.`);
        if (!Number.isInteger(b) || b < 1) throw new Error(`P${i + 1}: burst time must be a whole number, 1 or more.`);
        return { name: `P${i + 1}`, arrival: a, burst: b };
    });
    const q = Number($("#q").value);
    if (!Number.isInteger(q) || q < 1) throw new Error("Time quantum must be a whole number, 1 or more.");
    return { list, q };
}

/* ---------- Rendering ---------- */
function render(res) {
    const unit = 34;                              // px per time unit
    const start = res.gantt[0].start, end = res.gantt[res.gantt.length - 1].end;
    const g = $("#gantt"), t = $("#ticks");
    g.innerHTML = ""; t.innerHTML = "";
    g.style.width = t.style.width = Math.max((end - start) * unit, 300) + "px";
    const scale = Math.max((end - start) * unit, 300) / (end - start);

    res.gantt.forEach((s, i) => {
        const b = document.createElement("div");
        b.className = "blk" + (s.pid === "IDLE" ? " idle" : "");
        b.style.width = (s.end - s.start) * scale + "px";
        if (s.pid !== "IDLE") b.style.background = COLORS[s.idx % COLORS.length];
        b.style.animationDelay = i * 60 + "ms";
        b.textContent = s.pid;
        b.title = `${s.pid}: ${s.start} to ${s.end}`;
        g.appendChild(b);

        const k = document.createElement("div");
        k.className = "tick";
        k.style.width = (s.end - s.start) * scale + "px";
        k.innerHTML = `<span>${s.start}</span>`;
        t.appendChild(k);
    });
    const lastTick = document.createElement("div");
    lastTick.className = "tick last";
    lastTick.innerHTML = `<span>${end}</span>`;
    lastTick.style.cssText = "width:0;position:relative";
    t.appendChild(lastTick);

    const n = res.procs.length;
    const avg = k => (res.procs.reduce((a, p) => a + p[k], 0) / n).toFixed(2);
    $("#awt").textContent = avg("wt");
    $("#att").textContent = avg("tat");
    $("#cpu").textContent = res.util.toFixed(2) + "%";

    $("#result-table tbody").innerHTML = res.procs.map((p, i) =>
        `<tr><td><span class="chip" style="background:${COLORS[i % COLORS.length]}"></span>${p.name}</td>
     <td>${p.arrival}</td><td>${p.burst}</td><td>${p.completion}</td><td>${p.tat}</td><td>${p.wt}</td></tr>`).join("");
    $("#output").classList.remove("hidden");
}

/* ---------- Events ---------- */
$("#add").onclick = () => addRow();
$("#clear").onclick = () => { $("#input-table tbody").innerHTML = ""; $("#output").classList.add("hidden"); $("#error").textContent = ""; };
$("#sample").onclick = () => {
    $("#input-table tbody").innerHTML = "";
    [[0, 5], [1, 3], [2, 8], [3, 6]].forEach(([a, b]) => addRow(a, b));
    $("#q").value = 3; $("#error").textContent = "";
};
$("#run").onclick = () => {
    try {
        $("#error").textContent = "";
        const { list, q } = readInput();
        render(roundRobin(list, q));
    } catch (e) { $("#error").textContent = e.message; $("#output").classList.add("hidden"); }
};
$("#sample").click();
$("#run").click();