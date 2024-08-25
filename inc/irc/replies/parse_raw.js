let state = 0;
let replies = [];
require("fs").readFileSync("replies_raw", "utf-8").split("\n").forEach(function a(line) {
  let trimmed_line = line.trimLeft();
  const first_char = trimmed_line.substr(0, 1);
  const numeric = !isNaN(parseInt(first_char));
  //console.log(line);

  switch (state) {
    case 0:
      if (line.length == 0) { return; }
      if (!numeric) { console.log("expected reply"); process.exit(1); }
      replies.push({ code: parseInt(trimmed_line), mnemonic: trimmed_line.replace(/^[0-9]+ +/, ""), });
      state = 1;
      break;
    case 1:
      if (line.length == 0) { return; }
      if (numeric) { state = 0; return a(line); }
      if (first_char == '-') { state = 2; return a(line); }
      replies[replies.length - 1].example = (replies[replies.length - 1].example ?? "") + trimmed_line.replace(/\\/, "");
      break;
    case 2:
      if (numeric) { state = 0; return a(line); }
      if (first_char == '-') { trimmed_line = trimmed_line.substr(2); }
      replies[replies.length - 1].description = (replies[replies.length - 1].description ?? "") + trimmed_line + "\n";
      break;
  }
});

replies = replies.map(v => { if (v.description !== undefined) v.description = v.description.substr(0, v.description.length - 2); return v; });

//console.log(JSON.stringify(replies));

const enum_entries = replies.map(v => {
  const tab = "    ";
  const a = `${tab}${v.mnemonic} = ${v.code},\n`;
  const b = v.example !== undefined ? `${tab}/// example: ${v.example}\n` : "";
  const c = v.description !== undefined ? `${tab}/// description:\n${tab}///  ${v.description.replaceAll("\n", "\n" + tab + "///  ")}\n` : "";

  return `${b}${c}${a}\n`;
});

require("fs").writeFileSync("replies.ipp", `enum class numeric_reply : int {\n\n${enum_entries.join("")}};`);
require("fs").writeFileSync("reply_list.ipp", `static constexpr std::pair<int, const char*> valid_replies[] = { ${replies.sort((a, b)=>a.code-b.code).map(v=>`{${v.code}, "${v.mnemonic}"}`).join(", ")} };`)

