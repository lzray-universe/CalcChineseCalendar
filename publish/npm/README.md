# calcchinesecalendar

`calcchinesecalendar` ships the `lunar` engine as an ESM-oriented WebAssembly
package for Chinese calendar, almanac, and astronomy computations.

It is intended for installation from npm and direct use in Node.js or other ESM
environments.

The upstream C++ repository and the full reference documentation are available
at:

- GitHub: https://github.com/lzray-universe/CalcChineseCalendar/

If you need the full CLI reference, C++ API, C API, i18n details, repository
layout, or release artifacts, use the GitHub repository as the primary
reference.

## Installation

```bash
npm install calcchinesecalendar
```

## Quick start

```js
import { coreDay, createLunar } from "calcchinesecalendar";

const data = await coreDay({ ephem: "@series", date: "2025-06-01" });
console.log(data.data.phase_name);

const lunar = await createLunar();
const ganzhi = lunar.ganzhi({ ephem: "@series", date: "2025-09-07" });
console.log(ganzhi.data.day.text);
```

## Using BSP ephemerides

The `ephem` option selects the ephemeris source:

- use `@series` to force the built-in VSOP87A + ELPMPP02 fallback
- pass a mounted or accessible BSP path to use JPL DE ephemerides

For Node.js, a practical pattern is to write the BSP into the wasm filesystem
first:

```js
import { createLunar } from "calcchinesecalendar";
import { readFile } from "node:fs/promises";

const lunar = await createLunar();
const bsp = await readFile("de442.bsp");

await lunar.mkdir("/ephem");
await lunar.writeFile("/ephem/de442.bsp", bsp);

const data = lunar.day({ ephem: "/ephem/de442.bsp", date: "2025-06-01" });
console.log(data.data.phase_name);
```

## Public API overview

The package exposes:

- top-level async helpers such as `coreDay`, `ganzhi`, `day`, and `search`
- `createLunar()` for creating an isolated client with explicit state control
- native core APIs such as `coreDay`, `ganzhi`, `ganzhiMonth`, and `calcEot`
- high-level methods such as `day`, `monthview`, `at`, `convert`, `search`,
  `eclipse`, `festival`, `almanac`, and `info`
- raw `run` and `runJson` access
- filesystem helpers such as `writeFile`, `mkdir`, and `exists`

## Search example

```js
import { search } from "calcchinesecalendar";

const result = await search({
  ephem: "@series",
  query: "next full moon",
  fromTime: "2025-06-01T00:00:00+08:00",
  count: 2,
});

for (const item of result.data) {
  console.log(item.code, item.loc_iso);
}
```

## Notes

- npm package name: `calcchinesecalendar`
- The package is ESM-only
- Node.js 18 or newer is required
- The published package includes the wasm runtime files in `dist/`

For the full CLI, C++ API, C API, i18n documentation, and repository-level
usage notes, see the GitHub C++ repository:

- https://github.com/lzray-universe/CalcChineseCalendar/
