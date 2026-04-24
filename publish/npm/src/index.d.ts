export type CaptureResult = {
  code: number;
  stdout: string;
  stderr: string;
  error: string;
};

export class LunarError extends Error {
  code: number;
  stderr: string;
  stdout: string;
  error: string;
}

export type JsonCommandOptions = {
  pretty?: boolean;
  quiet?: boolean;
  extraArgs?: string[];
};

export type GanzhiRuleOptions = {
  trad?: string;
  yearBoundary?: string;
  monthBoundary?: string;
  leapMonthMode?: string;
  dayBoundary?: string;
};

export type CoreDayOptions = {
  ephem: string;
  date: string;
  tz?: string;
  pretty?: boolean;
};

export type CalcEotOptions = {
  ephem: string;
  jdUtc: number;
  lonDeg: number;
  pretty?: boolean;
};

export type GanzhiOptions = GanzhiRuleOptions & {
  ephem: string;
  date: string;
  atTime?: string;
  tz?: string;
  pretty?: boolean;
};

export type GanzhiMonthOptions = GanzhiRuleOptions & {
  ephem: string;
  year: number;
  month: number;
  atTime?: string;
  tz?: string;
  pretty?: boolean;
};

export type DayOptions = JsonCommandOptions & {
  ephem: string;
  date: string;
  tz?: string;
  lunarDayTz?: string;
  atTime?: string;
  events?: boolean;
  lon?: number;
  trad?: string;
  yearBoundary?: string;
  monthBoundary?: string;
  leapMonthMode?: string;
  dayBoundary?: string;
  astro?: boolean;
  astroMode?: string;
  astroPick?: string;
  astroLat?: number;
  astroLon?: number;
  astroHeight?: number;
};

export type MonthviewOptions = JsonCommandOptions & {
  ephem: string;
  yearMonth: string;
  tz?: string;
  lunarDayTz?: string;
  astro?: boolean;
  astroMode?: string;
  astroPick?: string;
  astroLat?: number;
  astroLon?: number;
  astroHeight?: number;
};

export type ExportDaysOptions = JsonCommandOptions & {
  ephem: string;
  yearMonth?: string;
  fromMonth?: string;
  toMonth?: string;
  fromYear?: number;
  toYear?: number;
  tz?: string;
  lunarDayTz?: string;
  atTime?: string;
  jobs?: number;
  events?: boolean;
  eclipse?: boolean;
  scope?: string;
  full?: boolean;
  huangli?: string;
  lon?: number;
  astro?: boolean;
  astroMode?: string;
  astroPick?: string;
  astroLat?: number;
  astroLon?: number;
  astroHeight?: number;
};

export type AtOptions = JsonCommandOptions & {
  ephem: string;
  time: string;
  inputTz?: string;
  tz?: string;
  lunarDayTz?: string;
  events?: boolean;
  eotLon?: number;
  trad?: string;
  yearBoundary?: string;
  monthBoundary?: string;
  leapMonthMode?: string;
  dayBoundary?: string;
};

export type ConvertOptions = JsonCommandOptions & {
  ephem: string;
  value: string;
  inputTz?: string;
  tz?: string;
  lunarDayTz?: string;
};

export type FromLunarOptions = JsonCommandOptions & {
  ephem: string;
  lunarYear: number;
  monthNo: number;
  lunarDay: number;
  leap?: boolean;
  tz?: string;
  lunarDayTz?: string;
};

export type NextOptions = JsonCommandOptions & {
  ephem: string;
  fromTime: string;
  count: number;
  kinds?: string;
  tz?: string;
  eclipse?: boolean;
};

export type RangeOptions = JsonCommandOptions & {
  ephem: string;
  fromTime: string;
  toTime: string;
  kinds?: string;
  tz?: string;
  eclipse?: boolean;
};

export type SearchOptions = JsonCommandOptions & {
  ephem: string;
  query: string;
  fromTime?: string;
  count?: number;
  tz?: string;
  eclipse?: boolean;
};

export type ZodiacOptions = JsonCommandOptions & {
  ephem: string;
  time?: string;
  year?: number;
  inputTz?: string;
  tz?: string;
};

export type SkyOptions = JsonCommandOptions & {
  ephem: string;
  time: string;
  lat: number;
  lon: number;
  height?: number;
  inputTz?: string;
  tz?: string;
  mode?: string;
  pick?: string;
};

export type EclipseOptions = JsonCommandOptions & {
  ephem: string;
  near?: string;
  visibleNear?: string;
  visibleYears?: number;
  kind?: string;
  stage?: string;
  sampleMin?: number;
  pointLat?: number;
  pointLon?: number;
  pointHeight?: number;
  pointRefine?: boolean;
  globalVis?: boolean;
  globalFormat?: string;
  gridLatStep?: number;
  gridLonStep?: number;
  tz?: string;
};

export type FestivalOptions = JsonCommandOptions & {
  ephem: string;
  year: number;
  tz?: string;
  lunarDayTz?: string;
};

export type AlmanacOptions = JsonCommandOptions & {
  ephem: string;
  date: string;
  tz?: string;
  lunarDayTz?: string;
  lon?: number;
  trad?: string;
  yearBoundary?: string;
  monthBoundary?: string;
  leapMonthMode?: string;
  dayBoundary?: string;
};

export type InfoOptions = JsonCommandOptions & {
  ephem: string;
};

export type CreateLunarOptions = {
  eclipseMethod?: string;
  lang?: string;
  module?: object;
};

export class LunarClient {
  constructor(module: object, options?: CreateLunarOptions);
  static create(options?: CreateLunarOptions): Promise<LunarClient>;
  version(): string;
  mkdir(path: string): void;
  exists(path: string): boolean;
  writeFile(path: string, data: ArrayBuffer | Uint8Array | ArrayBufferView): void;
  run(args: readonly string[], options?: { check?: boolean }): CaptureResult;
  runJson(args: readonly string[], options?: { check?: boolean }): unknown;
  coreDay(options: CoreDayOptions): unknown;
  calcEot(options: CalcEotOptions): unknown;
  ganzhi(options: GanzhiOptions): unknown;
  ganzhiMonth(options: GanzhiMonthOptions): unknown;
  day(options: DayOptions): unknown;
  monthview(options: MonthviewOptions): unknown;
  exportDays(options: ExportDaysOptions): unknown;
  at(options: AtOptions): unknown;
  convert(options: ConvertOptions): unknown;
  fromLunar(options: FromLunarOptions): unknown;
  nextEvents(options: NextOptions): unknown;
  rangeEvents(options: RangeOptions): unknown;
  search(options: SearchOptions): unknown;
  zodiac(options: ZodiacOptions): unknown;
  sky(options: SkyOptions): unknown;
  eclipse(options: EclipseOptions): unknown;
  festival(options: FestivalOptions): unknown;
  almanac(options: AlmanacOptions): unknown;
  info(options: InfoOptions): unknown;
}

export function createLunar(options?: CreateLunarOptions): Promise<LunarClient>;
export function run(args: readonly string[], options?: { check?: boolean }): Promise<CaptureResult>;
export function runJson(args: readonly string[], options?: { check?: boolean }): Promise<unknown>;
export function coreDay(options: CoreDayOptions): Promise<unknown>;
export function calcEot(options: CalcEotOptions): Promise<unknown>;
export function ganzhi(options: GanzhiOptions): Promise<unknown>;
export function ganzhiMonth(options: GanzhiMonthOptions): Promise<unknown>;
export function day(options: DayOptions): Promise<unknown>;
export function monthview(options: MonthviewOptions): Promise<unknown>;
export function exportDays(options: ExportDaysOptions): Promise<unknown>;
export function at(options: AtOptions): Promise<unknown>;
export function convert(options: ConvertOptions): Promise<unknown>;
export function fromLunar(options: FromLunarOptions): Promise<unknown>;
export function nextEvents(options: NextOptions): Promise<unknown>;
export function rangeEvents(options: RangeOptions): Promise<unknown>;
export function search(options: SearchOptions): Promise<unknown>;
export function zodiac(options: ZodiacOptions): Promise<unknown>;
export function sky(options: SkyOptions): Promise<unknown>;
export function eclipse(options: EclipseOptions): Promise<unknown>;
export function festival(options: FestivalOptions): Promise<unknown>;
export function almanac(options: AlmanacOptions): Promise<unknown>;
export function info(options: InfoOptions): Promise<unknown>;
