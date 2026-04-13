# calcchinesecalendar

`calcchinesecalendar` ships the `lunar` engine as a package-oriented
WebAssembly module.

```js
import { coreDay, createLunar } from "calcchinesecalendar";

const data = await coreDay({ ephem: "@series", date: "2025-06-01" });
console.log(data.data.phase_name);

const lunar = await createLunar();
console.log(lunar.ganzhi({ ephem: "@series", date: "2025-09-07" }).data.day.text);
```

The package exposes:

- top-level async helpers such as `coreDay`, `ganzhi`, `day`, and `search`
- `createLunar()` for creating an isolated client when you want explicit control
- native core APIs such as `coreDay`, `ganzhi`, `ganzhiMonth`, and `calcEot`
- high-level methods such as `day`, `monthview`, `at`, `convert`, `search`,
  `eclipse`, `festival`, `almanac`, and `info`
- raw `run` / `runJson` access
- file helpers such as `writeFile`, `mkdir`, and `exists`
