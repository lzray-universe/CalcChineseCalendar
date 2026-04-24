import createLunarModule from "../dist/lunar_api.js";

function appendValue(args, flag, value) {
  if (value === undefined || value === null) {
    return;
  }
  args.push(flag, String(value));
}

function appendBool(args, flag, value) {
  if (value === undefined || value === null) {
    return;
  }
  args.push(flag, value ? "1" : "0");
}

function appendMany(args, extraArgs) {
  if (!Array.isArray(extraArgs)) {
    return;
  }
  for (const item of extraArgs) {
    args.push(String(item));
  }
}

function toUint8Array(data) {
  if (data instanceof Uint8Array) {
    return data;
  }
  if (data instanceof ArrayBuffer) {
    return new Uint8Array(data);
  }
  if (ArrayBuffer.isView(data)) {
    return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
  }
  throw new TypeError("file data must be ArrayBuffer or Uint8Array");
}

function normalizePath(path) {
  return String(path).replace(/\\/g, "/");
}

function dirname(path) {
  const text = normalizePath(path);
  const pos = text.lastIndexOf("/");
  if (pos <= 0) {
    return "/";
  }
  return text.slice(0, pos);
}

function joinPath(base, name) {
  if (base === "/" || base.length === 0) {
    return `/${name}`;
  }
  return base.endsWith("/") ? `${base}${name}` : `${base}/${name}`;
}

function ensureDir(module, path) {
  const text = normalizePath(path);
  if (text === "/" || text.length === 0) {
    return;
  }
  const parts = text.split("/");
  let current = "";
  for (const part of parts) {
    if (!part || part === "." || part === "..") {
      continue;
    }
    current = joinPath(current || "/", part);
    if (!module.FS.analyzePath(current).exists) {
      module.FS.mkdir(current);
    }
  }
}

function readCString(module, ptr) {
  if (!ptr) {
    return "";
  }
  return module.UTF8ToString(ptr);
}

function allocCString(module, value) {
  if (value === undefined || value === null) {
    return 0;
  }
  const text = String(value);
  const size = module.lengthBytesUTF8(text) + 1;
  const ptr = module._malloc(size);
  module.stringToUTF8(text, ptr, size);
  return ptr;
}

function withCStringArgs(module, values, fn) {
  const ptrs = values.map((value) => allocCString(module, value));
  try {
    return fn(...ptrs);
  } finally {
    for (const ptr of ptrs) {
      if (ptr) {
        module._free(ptr);
      }
    }
  }
}

function parseJsonResult(module, code) {
  const stdout = readCString(module, module._lunar_last_stdout());
  const stderr = readCString(module, module._lunar_last_stderr());
  const error = readCString(module, module._lunar_last_error());
  const result = { code, stdout, stderr, error };
  if (code !== 0) {
    throw new LunarError(result);
  }
  return JSON.parse(stdout);
}

function ruleCodes(options = {}) {
  const profileCodes = new Map([
    [undefined, 0],
    [null, 0],
    ["folk", 0],
    ["ziping", 1],
    ["purple", 2],
    ["xieji", 3],
  ]);
  const yearBoundaryCodes = new Map([
    [undefined, 1],
    [null, 1],
    ["lichun", 0],
    ["lunar_new_year", 1],
    ["dongzhi", 2],
  ]);
  const monthBoundaryCodes = new Map([
    [undefined, 1],
    [null, 1],
    ["solar_term", 0],
    ["lunar_first_day", 1],
  ]);
  const leapMonthModeCodes = new Map([
    [undefined, 1],
    [null, 1],
    ["ignore", 0],
    ["inherit_previous", 1],
    ["split_midway", 2],
    ["shift_to_next", 3],
  ]);
  const dayBoundaryCodes = new Map([
    [undefined, 0],
    [null, 0],
    ["hour23", 0],
    ["hour0", 1],
  ]);
  const codes = [
    profileCodes.get(options.trad),
    yearBoundaryCodes.get(options.yearBoundary),
    monthBoundaryCodes.get(options.monthBoundary),
    leapMonthModeCodes.get(options.leapMonthMode),
    dayBoundaryCodes.get(options.dayBoundary),
  ];
  if (codes.some((value) => value === undefined)) {
    throw new RangeError("invalid ganzhi rule option");
  }
  return codes;
}

function callCapture(module, args) {
  const ptrs = [];
  let argvPtr = 0;
  try {
    for (const arg of args) {
      const size = module.lengthBytesUTF8(arg) + 1;
      const ptr = module._malloc(size);
      module.stringToUTF8(arg, ptr, size);
      ptrs.push(ptr);
    }
    argvPtr = module._malloc(ptrs.length * 4);
    for (let i = 0; i < ptrs.length; i += 1) {
      module.HEAPU32[(argvPtr >> 2) + i] = ptrs[i];
    }
    const code = module._lunar_run_capture(ptrs.length, argvPtr);
    const stdout = readCString(module, module._lunar_last_stdout());
    const stderr = readCString(module, module._lunar_last_stderr());
    const error = readCString(module, module._lunar_last_error());
    return { code, stdout, stderr, error };
  } finally {
    if (argvPtr) {
      module._free(argvPtr);
    }
    for (const ptr of ptrs) {
      module._free(ptr);
    }
  }
}

export class LunarError extends Error {
  constructor(result) {
    const message =
      result.stderr.trim() ||
      result.error.trim() ||
      `lunar command failed with exit code ${result.code}`;
    super(message);
    this.name = "LunarError";
    this.code = result.code;
    this.stderr = result.stderr;
    this.stdout = result.stdout;
    this.error = result.error;
  }
}

export class LunarClient {
  constructor(module, options = {}) {
    this._module = module;
    this._lang = options.lang ?? null;
    this._eclipseMethod = options.eclipseMethod ?? null;
  }

  static async create(options = {}) {
    const module = await createLunarModule(options.module ?? {});
    return new LunarClient(module, options);
  }

  version() {
    return readCString(this._module, this._module._lunar_tool_ver());
  }

  mkdir(path) {
    ensureDir(this._module, path);
  }

  exists(path) {
    return this._module.FS.analyzePath(normalizePath(path)).exists;
  }

  writeFile(path, data) {
    const text = normalizePath(path);
    ensureDir(this._module, dirname(text));
    this._module.FS.writeFile(text, toUint8Array(data));
  }

  _compose(args) {
    const out = [];
    appendValue(out, "--lang", this._lang);
    appendValue(out, "--eclipse-method", this._eclipseMethod);
    for (const arg of args) {
      out.push(String(arg));
    }
    return out;
  }

  run(args, options = {}) {
    const result = callCapture(this._module, this._compose(args));
    if (options.check !== false && result.code !== 0) {
      throw new LunarError(result);
    }
    return result;
  }

  runJson(args, options = {}) {
    const result = this.run(args, options);
    return JSON.parse(result.stdout);
  }

  coreDay(options) {
    const code = withCStringArgs(
      this._module,
      [options.ephem, options.date, options.tz ?? "+08:00"],
      (ephemPtr, datePtr, tzPtr) =>
        this._module._lunar_core_day_json(
          ephemPtr,
          datePtr,
          tzPtr,
          options.pretty ? 1 : 0,
        ),
    );
    return parseJsonResult(this._module, code);
  }

  calcEot(options) {
    const code = withCStringArgs(this._module, [options.ephem], (ephemPtr) =>
      this._module._lunar_calc_eot_json(
        ephemPtr,
        Number(options.jdUtc),
        Number(options.lonDeg),
        options.pretty ? 1 : 0,
      ),
    );
    return parseJsonResult(this._module, code);
  }

  ganzhi(options) {
    const codes = ruleCodes(options);
    const code = withCStringArgs(
      this._module,
      [
        options.ephem,
        options.date,
        options.atTime ?? "12:00:00",
        options.tz ?? "+08:00",
      ],
      (ephemPtr, datePtr, atTimePtr, tzPtr) =>
        this._module._lunar_core_ganzhi_json(
          ephemPtr,
          datePtr,
          atTimePtr,
          tzPtr,
          ...codes,
          options.pretty ? 1 : 0,
        ),
    );
    return parseJsonResult(this._module, code);
  }

  ganzhiMonth(options) {
    const codes = ruleCodes(options);
    const code = withCStringArgs(
      this._module,
      [
        options.ephem,
        options.atTime ?? "12:00:00",
        options.tz ?? "+08:00",
      ],
      (ephemPtr, atTimePtr, tzPtr) =>
        this._module._lunar_core_ganzhi_month_json(
          ephemPtr,
          Number(options.year),
          Number(options.month),
          atTimePtr,
          tzPtr,
          ...codes,
          options.pretty ? 1 : 0,
        ),
    );
    return parseJsonResult(this._module, code);
  }

  _jsonCommand(args, options = {}) {
    appendMany(args, options.extraArgs);
    args.push("--format", "json", "--pretty", options.pretty ? "1" : "0");
    if (options.quiet !== false) {
      args.push("--quiet");
    }
    return this.runJson(args);
  }

  day(options) {
    const args = ["day", options.ephem, options.date];
    appendValue(args, "--tz", options.tz);
    appendValue(args, "--lunar-day-tz", options.lunarDayTz);
    appendValue(args, "--at", options.atTime);
    appendBool(args, "--events", options.events);
    appendValue(args, "--lon", options.lon);
    appendValue(args, "--trad", options.trad);
    appendValue(args, "--year-boundary", options.yearBoundary);
    appendValue(args, "--month-boundary", options.monthBoundary);
    appendValue(args, "--leap-month-mode", options.leapMonthMode);
    appendValue(args, "--day-boundary", options.dayBoundary);
    appendBool(args, "--astro", options.astro);
    appendValue(args, "--astro-mode", options.astroMode);
    appendValue(args, "--astro-pick", options.astroPick);
    appendValue(args, "--astro-lat", options.astroLat);
    appendValue(args, "--astro-lon", options.astroLon);
    appendValue(args, "--astro-height", options.astroHeight);
    return this._jsonCommand(args, options);
  }

  monthview(options) {
    const args = ["monthview", options.ephem, options.yearMonth];
    appendValue(args, "--tz", options.tz);
    appendValue(args, "--lunar-day-tz", options.lunarDayTz);
    appendBool(args, "--astro", options.astro);
    appendValue(args, "--astro-mode", options.astroMode);
    appendValue(args, "--astro-pick", options.astroPick);
    appendValue(args, "--astro-lat", options.astroLat);
    appendValue(args, "--astro-lon", options.astroLon);
    appendValue(args, "--astro-height", options.astroHeight);
    return this._jsonCommand(args, options);
  }

  exportDays(options) {
    const args = ["export", options.ephem];
    if (options.yearMonth !== undefined && options.yearMonth !== null) {
      args.push(String(options.yearMonth));
    }
    appendValue(args, "--from", options.fromMonth);
    appendValue(args, "--to", options.toMonth);
    appendValue(args, "--from-year", options.fromYear);
    appendValue(args, "--to-year", options.toYear);
    appendValue(args, "--tz", options.tz);
    appendValue(args, "--lunar-day-tz", options.lunarDayTz);
    appendValue(args, "--at", options.atTime);
    appendValue(args, "--jobs", options.jobs);
    appendBool(args, "--events", options.events);
    appendBool(args, "--eclipse", options.eclipse);
    appendValue(args, "--scope", options.scope);
    appendBool(args, "--full", options.full);
    appendValue(args, "--huangli", options.huangli);
    appendValue(args, "--lon", options.lon);
    appendBool(args, "--astro", options.astro);
    appendValue(args, "--astro-mode", options.astroMode);
    appendValue(args, "--astro-pick", options.astroPick);
    appendValue(args, "--astro-lat", options.astroLat);
    appendValue(args, "--astro-lon", options.astroLon);
    appendValue(args, "--astro-height", options.astroHeight);
    return this._jsonCommand(args, options);
  }

  at(options) {
    const args = ["at", options.ephem, options.time];
    appendValue(args, "--input-tz", options.inputTz);
    appendValue(args, "--tz", options.tz);
    appendValue(args, "--lunar-day-tz", options.lunarDayTz);
    appendBool(args, "--events", options.events);
    appendValue(args, "--eot-lon", options.eotLon);
    appendValue(args, "--trad", options.trad);
    appendValue(args, "--year-boundary", options.yearBoundary);
    appendValue(args, "--month-boundary", options.monthBoundary);
    appendValue(args, "--leap-month-mode", options.leapMonthMode);
    appendValue(args, "--day-boundary", options.dayBoundary);
    return this._jsonCommand(args, options);
  }

  convert(options) {
    const args = ["convert", options.ephem, options.value];
    appendValue(args, "--input-tz", options.inputTz);
    appendValue(args, "--tz", options.tz);
    appendValue(args, "--lunar-day-tz", options.lunarDayTz);
    return this._jsonCommand(args, options);
  }

  fromLunar(options) {
    const args = [
      "convert",
      options.ephem,
      "--from-lunar",
      options.lunarYear,
      options.monthNo,
      options.lunarDay,
    ];
    appendBool(args, "--leap", options.leap);
    appendValue(args, "--tz", options.tz);
    appendValue(args, "--lunar-day-tz", options.lunarDayTz);
    return this._jsonCommand(args, options);
  }

  nextEvents(options) {
    const args = [
      "next",
      options.ephem,
      "--from",
      options.fromTime,
      "--count",
      options.count,
    ];
    appendValue(args, "--kinds", options.kinds);
    appendValue(args, "--tz", options.tz);
    appendBool(args, "--eclipse", options.eclipse);
    return this._jsonCommand(args, options);
  }

  rangeEvents(options) {
    const args = [
      "range",
      options.ephem,
      "--from",
      options.fromTime,
      "--to",
      options.toTime,
    ];
    appendValue(args, "--kinds", options.kinds);
    appendValue(args, "--tz", options.tz);
    appendBool(args, "--eclipse", options.eclipse);
    return this._jsonCommand(args, options);
  }

  search(options) {
    const args = ["search", options.ephem, options.query];
    appendValue(args, "--from", options.fromTime);
    appendValue(args, "--count", options.count);
    appendValue(args, "--tz", options.tz);
    appendBool(args, "--eclipse", options.eclipse);
    return this._jsonCommand(args, options);
  }

  zodiac(options) {
    const args = ["zodiac", options.ephem];
    appendValue(args, "--time", options.time);
    appendValue(args, "--year", options.year);
    appendValue(args, "--input-tz", options.inputTz);
    appendValue(args, "--tz", options.tz);
    return this._jsonCommand(args, options);
  }

  sky(options) {
    const args = [
      "sky",
      options.ephem,
      options.time,
      "--lat",
      options.lat,
      "--lon",
      options.lon,
    ];
    appendValue(args, "--height", options.height);
    appendValue(args, "--input-tz", options.inputTz);
    appendValue(args, "--tz", options.tz);
    appendValue(args, "--mode", options.mode);
    appendValue(args, "--pick", options.pick);
    return this._jsonCommand(args, options);
  }

  eclipse(options) {
    const args = ["eclipse", options.ephem];
    appendValue(args, "--near", options.near);
    appendValue(args, "--visible-near", options.visibleNear);
    appendValue(args, "--visible-years", options.visibleYears);
    appendValue(args, "--kind", options.kind);
    appendValue(args, "--stage", options.stage);
    appendValue(args, "--sample-min", options.sampleMin);
    appendValue(args, "--point-lat", options.pointLat);
    appendValue(args, "--point-lon", options.pointLon);
    appendValue(args, "--point-height", options.pointHeight);
    appendBool(args, "--point-refine", options.pointRefine);
    appendBool(args, "--global-vis", options.globalVis);
    appendValue(args, "--global-format", options.globalFormat);
    appendValue(args, "--grid-lat-step", options.gridLatStep);
    appendValue(args, "--grid-lon-step", options.gridLonStep);
    appendValue(args, "--tz", options.tz);
    return this._jsonCommand(args, options);
  }

  festival(options) {
    const args = ["festival", options.ephem, options.year];
    appendValue(args, "--tz", options.tz);
    appendValue(args, "--lunar-day-tz", options.lunarDayTz);
    return this._jsonCommand(args, options);
  }

  almanac(options) {
    const args = ["almanac", options.ephem, options.date];
    appendValue(args, "--tz", options.tz);
    appendValue(args, "--lunar-day-tz", options.lunarDayTz);
    appendValue(args, "--lon", options.lon);
    appendValue(args, "--trad", options.trad);
    appendValue(args, "--year-boundary", options.yearBoundary);
    appendValue(args, "--month-boundary", options.monthBoundary);
    appendValue(args, "--leap-month-mode", options.leapMonthMode);
    appendValue(args, "--day-boundary", options.dayBoundary);
    return this._jsonCommand(args, options);
  }

  info(options) {
    const args = ["info", options.ephem];
    return this._jsonCommand(args, options);
  }
}

let defaultClientPromise = null;

async function getDefaultClient() {
  if (defaultClientPromise === null) {
    defaultClientPromise = createLunar();
  }
  return defaultClientPromise;
}

export async function createLunar(options = {}) {
  return LunarClient.create(options);
}

export async function run(args, options = {}) {
  return (await getDefaultClient()).run(args, options);
}

export async function runJson(args, options = {}) {
  return (await getDefaultClient()).runJson(args, options);
}

export async function coreDay(options) {
  return (await getDefaultClient()).coreDay(options);
}

export async function calcEot(options) {
  return (await getDefaultClient()).calcEot(options);
}

export async function ganzhi(options) {
  return (await getDefaultClient()).ganzhi(options);
}

export async function ganzhiMonth(options) {
  return (await getDefaultClient()).ganzhiMonth(options);
}

export async function day(options) {
  return (await getDefaultClient()).day(options);
}

export async function monthview(options) {
  return (await getDefaultClient()).monthview(options);
}

export async function exportDays(options) {
  return (await getDefaultClient()).exportDays(options);
}

export async function at(options) {
  return (await getDefaultClient()).at(options);
}

export async function convert(options) {
  return (await getDefaultClient()).convert(options);
}

export async function fromLunar(options) {
  return (await getDefaultClient()).fromLunar(options);
}

export async function nextEvents(options) {
  return (await getDefaultClient()).nextEvents(options);
}

export async function rangeEvents(options) {
  return (await getDefaultClient()).rangeEvents(options);
}

export async function search(options) {
  return (await getDefaultClient()).search(options);
}

export async function zodiac(options) {
  return (await getDefaultClient()).zodiac(options);
}

export async function sky(options) {
  return (await getDefaultClient()).sky(options);
}

export async function eclipse(options) {
  return (await getDefaultClient()).eclipse(options);
}

export async function festival(options) {
  return (await getDefaultClient()).festival(options);
}

export async function almanac(options) {
  return (await getDefaultClient()).almanac(options);
}

export async function info(options) {
  return (await getDefaultClient()).info(options);
}
