from __future__ import annotations

from dataclasses import dataclass
import importlib
import json
from typing import Any
from typing import Iterable
from typing import Sequence

try:
    _lunar_ext = importlib.import_module("._lunar_ext", __package__)
except ImportError:
    _lunar_ext = importlib.import_module("_lunar_ext")
from ._version import __version__


JsonValue = Any


class LunarError(RuntimeError):
    def __init__(self, code: int, stderr: str, stdout: str = "", error: str = "") -> None:
        message = stderr.strip() or error.strip() or f"lunar command failed with exit code {code}"
        super().__init__(message)
        self.code = code
        self.stderr = stderr
        self.stdout = stdout
        self.error = error


@dataclass(frozen=True)
class CaptureResult:
    code: int
    stdout: str
    stderr: str
    error: str = ""


def _append_value(args: list[str], flag: str, value: Any) -> None:
    if value is None:
        return
    args.extend((flag, str(value)))


def _append_bool(args: list[str], flag: str, value: bool | None) -> None:
    if value is None:
        return
    args.extend((flag, "1" if value else "0"))


def _append_many(args: list[str], extra_args: Iterable[str] | None) -> None:
    if extra_args is None:
        return
    for item in extra_args:
        args.append(str(item))


def _rule_codes(
    trad: str | None = None,
    year_boundary: str | None = None,
    month_boundary: str | None = None,
    leap_month_mode: str | None = None,
    day_boundary: str | None = None,
) -> tuple[int, int, int, int, int]:
    profile_codes = {
        None: 0,
        "folk": 0,
        "ziping": 1,
        "purple": 2,
        "xieji": 3,
    }
    year_boundary_codes = {
        None: 1,
        "lichun": 0,
        "lunar_new_year": 1,
        "dongzhi": 2,
    }
    month_boundary_codes = {
        None: 1,
        "solar_term": 0,
        "lunar_first_day": 1,
    }
    leap_month_mode_codes = {
        None: 1,
        "ignore": 0,
        "inherit_previous": 1,
        "split_midway": 2,
        "shift_to_next": 3,
    }
    day_boundary_codes = {
        None: 0,
        "hour23": 0,
        "hour0": 1,
    }
    try:
        return (
            profile_codes[trad],
            year_boundary_codes[year_boundary],
            month_boundary_codes[month_boundary],
            leap_month_mode_codes[leap_month_mode],
            day_boundary_codes[day_boundary],
        )
    except KeyError as exc:
        raise ValueError(f"invalid rule option: {exc.args[0]}") from exc


class Lunar:
    def __init__(
        self,
        *,
        lang: str | None = None,
        eclipse_method: str | None = None,
    ) -> None:
        self._lang = lang
        self._eclipse_method = eclipse_method

    def _compose(self, args: Sequence[str]) -> list[str]:
        out: list[str] = []
        _append_value(out, "--lang", self._lang)
        _append_value(out, "--eclipse-method", self._eclipse_method)
        out.extend(str(arg) for arg in args)
        return out

    def run(self, args: Sequence[str], *, check: bool = True) -> CaptureResult:
        code, stdout, stderr, error = _lunar_ext.run_capture(self._compose(args))
        result = CaptureResult(code=code, stdout=stdout, stderr=stderr, error=error)
        if check and result.code != 0:
            raise LunarError(result.code, result.stderr, result.stdout, result.error)
        return result

    def run_json(self, args: Sequence[str], *, check: bool = True) -> JsonValue:
        result = self.run(args, check=check)
        if result.code != 0:
            return None
        return json.loads(result.stdout)

    def _json_command(
        self,
        args: list[str],
        *,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        _append_many(args, extra_args)
        args.extend(("--format", "json", "--pretty", "1" if pretty else "0"))
        if quiet:
            args.append("--quiet")
        return self.run_json(args)

    def day(
        self,
        ephem: str,
        date: str,
        *,
        tz: str | None = None,
        lunar_day_tz: str | None = None,
        at_time: str | None = None,
        events: bool | None = None,
        lon: float | None = None,
        trad: str | None = None,
        year_boundary: str | None = None,
        month_boundary: str | None = None,
        leap_month_mode: str | None = None,
        day_boundary: str | None = None,
        astro: bool | None = None,
        astro_mode: str | None = None,
        astro_pick: str | None = None,
        astro_lat: float | None = None,
        astro_lon: float | None = None,
        astro_height: float | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["day", ephem, date]
        _append_value(args, "--tz", tz)
        _append_value(args, "--lunar-day-tz", lunar_day_tz)
        _append_value(args, "--at", at_time)
        _append_bool(args, "--events", events)
        _append_value(args, "--lon", lon)
        _append_value(args, "--trad", trad)
        _append_value(args, "--year-boundary", year_boundary)
        _append_value(args, "--month-boundary", month_boundary)
        _append_value(args, "--leap-month-mode", leap_month_mode)
        _append_value(args, "--day-boundary", day_boundary)
        _append_bool(args, "--astro", astro)
        _append_value(args, "--astro-mode", astro_mode)
        _append_value(args, "--astro-pick", astro_pick)
        _append_value(args, "--astro-lat", astro_lat)
        _append_value(args, "--astro-lon", astro_lon)
        _append_value(args, "--astro-height", astro_height)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def core_day(
        self,
        ephem: str,
        date: str,
        *,
        tz: str = "+08:00",
        pretty: bool = False,
    ) -> JsonValue:
        return json.loads(_lunar_ext.core_day_json(ephem, date, tz, pretty))

    def calc_eot(
        self,
        ephem: str,
        jd_utc: float,
        lon_deg: float,
        *,
        pretty: bool = False,
    ) -> JsonValue:
        return json.loads(_lunar_ext.calc_eot_json(ephem, jd_utc, lon_deg, pretty))

    def ganzhi(
        self,
        ephem: str,
        date: str,
        *,
        at_time: str = "12:00:00",
        tz: str = "+08:00",
        trad: str | None = None,
        year_boundary: str | None = None,
        month_boundary: str | None = None,
        leap_month_mode: str | None = None,
        day_boundary: str | None = None,
        pretty: bool = False,
    ) -> JsonValue:
        rule_codes = _rule_codes(
            trad,
            year_boundary,
            month_boundary,
            leap_month_mode,
            day_boundary,
        )
        return json.loads(
            _lunar_ext.core_ganzhi_json(
                ephem,
                date,
                at_time,
                tz,
                *rule_codes,
                pretty,
            )
        )

    def ganzhi_month(
        self,
        ephem: str,
        year: int,
        month: int,
        *,
        at_time: str = "12:00:00",
        tz: str = "+08:00",
        trad: str | None = None,
        year_boundary: str | None = None,
        month_boundary: str | None = None,
        leap_month_mode: str | None = None,
        day_boundary: str | None = None,
        pretty: bool = False,
    ) -> JsonValue:
        rule_codes = _rule_codes(
            trad,
            year_boundary,
            month_boundary,
            leap_month_mode,
            day_boundary,
        )
        return json.loads(
            _lunar_ext.core_ganzhi_month_json(
                ephem,
                year,
                month,
                at_time,
                tz,
                *rule_codes,
                pretty,
            )
        )

    def monthview(
        self,
        ephem: str,
        year_month: str,
        *,
        tz: str | None = None,
        lunar_day_tz: str | None = None,
        astro: bool | None = None,
        astro_mode: str | None = None,
        astro_pick: str | None = None,
        astro_lat: float | None = None,
        astro_lon: float | None = None,
        astro_height: float | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["monthview", ephem, year_month]
        _append_value(args, "--tz", tz)
        _append_value(args, "--lunar-day-tz", lunar_day_tz)
        _append_bool(args, "--astro", astro)
        _append_value(args, "--astro-mode", astro_mode)
        _append_value(args, "--astro-pick", astro_pick)
        _append_value(args, "--astro-lat", astro_lat)
        _append_value(args, "--astro-lon", astro_lon)
        _append_value(args, "--astro-height", astro_height)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def export_days(
        self,
        ephem: str,
        *,
        year_month: str | None = None,
        from_month: str | None = None,
        to_month: str | None = None,
        from_year: int | None = None,
        to_year: int | None = None,
        tz: str | None = None,
        lunar_day_tz: str | None = None,
        at_time: str | None = None,
        jobs: int | None = None,
        events: bool | None = None,
        eclipse: bool | None = None,
        scope: str | None = None,
        full: bool | None = None,
        huangli: str | None = None,
        lon: float | None = None,
        astro: bool | None = None,
        astro_mode: str | None = None,
        astro_pick: str | None = None,
        astro_lat: float | None = None,
        astro_lon: float | None = None,
        astro_height: float | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["export", ephem]
        if year_month is not None:
            args.append(str(year_month))
        _append_value(args, "--from", from_month)
        _append_value(args, "--to", to_month)
        _append_value(args, "--from-year", from_year)
        _append_value(args, "--to-year", to_year)
        _append_value(args, "--tz", tz)
        _append_value(args, "--lunar-day-tz", lunar_day_tz)
        _append_value(args, "--at", at_time)
        _append_value(args, "--jobs", jobs)
        _append_bool(args, "--events", events)
        _append_bool(args, "--eclipse", eclipse)
        _append_value(args, "--scope", scope)
        _append_bool(args, "--full", full)
        _append_value(args, "--huangli", huangli)
        _append_value(args, "--lon", lon)
        _append_bool(args, "--astro", astro)
        _append_value(args, "--astro-mode", astro_mode)
        _append_value(args, "--astro-pick", astro_pick)
        _append_value(args, "--astro-lat", astro_lat)
        _append_value(args, "--astro-lon", astro_lon)
        _append_value(args, "--astro-height", astro_height)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def at(
        self,
        ephem: str,
        time: str,
        *,
        input_tz: str | None = None,
        tz: str | None = None,
        lunar_day_tz: str | None = None,
        events: bool | None = None,
        eot_lon: float | None = None,
        trad: str | None = None,
        year_boundary: str | None = None,
        month_boundary: str | None = None,
        leap_month_mode: str | None = None,
        day_boundary: str | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["at", ephem, time]
        _append_value(args, "--input-tz", input_tz)
        _append_value(args, "--tz", tz)
        _append_value(args, "--lunar-day-tz", lunar_day_tz)
        _append_bool(args, "--events", events)
        _append_value(args, "--eot-lon", eot_lon)
        _append_value(args, "--trad", trad)
        _append_value(args, "--year-boundary", year_boundary)
        _append_value(args, "--month-boundary", month_boundary)
        _append_value(args, "--leap-month-mode", leap_month_mode)
        _append_value(args, "--day-boundary", day_boundary)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def convert(
        self,
        ephem: str,
        value: str,
        *,
        input_tz: str | None = None,
        tz: str | None = None,
        lunar_day_tz: str | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["convert", ephem, value]
        _append_value(args, "--input-tz", input_tz)
        _append_value(args, "--tz", tz)
        _append_value(args, "--lunar-day-tz", lunar_day_tz)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def from_lunar(
        self,
        ephem: str,
        lunar_year: int,
        month_no: int,
        lunar_day: int,
        *,
        leap: bool | None = None,
        tz: str | None = None,
        lunar_day_tz: str | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = [
            "convert",
            ephem,
            "--from-lunar",
            str(lunar_year),
            str(month_no),
            str(lunar_day),
        ]
        _append_bool(args, "--leap", leap)
        _append_value(args, "--tz", tz)
        _append_value(args, "--lunar-day-tz", lunar_day_tz)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def next_events(
        self,
        ephem: str,
        from_time: str,
        count: int,
        *,
        kinds: str | None = None,
        tz: str | None = None,
        eclipse: bool | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["next", ephem, "--from", from_time, "--count", str(count)]
        _append_value(args, "--kinds", kinds)
        _append_value(args, "--tz", tz)
        _append_bool(args, "--eclipse", eclipse)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def range_events(
        self,
        ephem: str,
        from_time: str,
        to_time: str,
        *,
        kinds: str | None = None,
        tz: str | None = None,
        eclipse: bool | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["range", ephem, "--from", from_time, "--to", to_time]
        _append_value(args, "--kinds", kinds)
        _append_value(args, "--tz", tz)
        _append_bool(args, "--eclipse", eclipse)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def search(
        self,
        ephem: str,
        query: str,
        *,
        from_time: str | None = None,
        count: int | None = None,
        tz: str | None = None,
        eclipse: bool | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["search", ephem, query]
        _append_value(args, "--from", from_time)
        _append_value(args, "--count", count)
        _append_value(args, "--tz", tz)
        _append_bool(args, "--eclipse", eclipse)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def zodiac(
        self,
        ephem: str,
        *,
        time: str | None = None,
        year: int | None = None,
        input_tz: str | None = None,
        tz: str | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["zodiac", ephem]
        _append_value(args, "--time", time)
        _append_value(args, "--year", year)
        _append_value(args, "--input-tz", input_tz)
        _append_value(args, "--tz", tz)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def sky(
        self,
        ephem: str,
        time: str,
        *,
        lat: float,
        lon: float,
        height: float | None = None,
        input_tz: str | None = None,
        tz: str | None = None,
        mode: str | None = None,
        pick: str | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["sky", ephem, time, "--lat", str(lat), "--lon", str(lon)]
        _append_value(args, "--height", height)
        _append_value(args, "--input-tz", input_tz)
        _append_value(args, "--tz", tz)
        _append_value(args, "--mode", mode)
        _append_value(args, "--pick", pick)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def eclipse(
        self,
        ephem: str,
        near: str | None = None,
        *,
        visible_near: str | None = None,
        visible_years: int | None = None,
        kind: str | None = None,
        stage: str | None = None,
        sample_min: float | None = None,
        point_lat: float | None = None,
        point_lon: float | None = None,
        point_height: float | None = None,
        point_refine: bool | None = None,
        global_vis: bool | None = None,
        global_format: str | None = None,
        grid_lat_step: float | None = None,
        grid_lon_step: float | None = None,
        tz: str | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["eclipse", ephem]
        _append_value(args, "--near", near)
        _append_value(args, "--visible-near", visible_near)
        _append_value(args, "--visible-years", visible_years)
        _append_value(args, "--kind", kind)
        _append_value(args, "--stage", stage)
        _append_value(args, "--sample-min", sample_min)
        _append_value(args, "--point-lat", point_lat)
        _append_value(args, "--point-lon", point_lon)
        _append_value(args, "--point-height", point_height)
        _append_bool(args, "--point-refine", point_refine)
        _append_bool(args, "--global-vis", global_vis)
        _append_value(args, "--global-format", global_format)
        _append_value(args, "--grid-lat-step", grid_lat_step)
        _append_value(args, "--grid-lon-step", grid_lon_step)
        _append_value(args, "--tz", tz)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def festival(
        self,
        ephem: str,
        year: int,
        *,
        tz: str | None = None,
        lunar_day_tz: str | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["festival", ephem, str(year)]
        _append_value(args, "--tz", tz)
        _append_value(args, "--lunar-day-tz", lunar_day_tz)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def almanac(
        self,
        ephem: str,
        date: str,
        *,
        tz: str | None = None,
        lunar_day_tz: str | None = None,
        lon: float | None = None,
        trad: str | None = None,
        year_boundary: str | None = None,
        month_boundary: str | None = None,
        leap_month_mode: str | None = None,
        day_boundary: str | None = None,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["almanac", ephem, date]
        _append_value(args, "--tz", tz)
        _append_value(args, "--lunar-day-tz", lunar_day_tz)
        _append_value(args, "--lon", lon)
        _append_value(args, "--trad", trad)
        _append_value(args, "--year-boundary", year_boundary)
        _append_value(args, "--month-boundary", month_boundary)
        _append_value(args, "--leap-month-mode", leap_month_mode)
        _append_value(args, "--day-boundary", day_boundary)
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)

    def info(
        self,
        ephem: str,
        *,
        pretty: bool = False,
        quiet: bool = True,
        extra_args: Iterable[str] | None = None,
    ) -> JsonValue:
        args = ["info", ephem]
        return self._json_command(args, pretty=pretty, quiet=quiet, extra_args=extra_args)


_DEFAULT=Lunar()


def version() -> str:
    return _lunar_ext.tool_version() or __version__


def run(args: Sequence[str], *, check: bool = True) -> CaptureResult:
    return _DEFAULT.run(args, check=check)


def run_json(args: Sequence[str], *, check: bool = True) -> JsonValue:
    return _DEFAULT.run_json(args, check=check)


def day(ephem: str, date: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.day(ephem, date, **kwargs)


def core_day(ephem: str, date: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.core_day(ephem, date, **kwargs)


def calc_eot(ephem: str, jd_utc: float, lon_deg: float, **kwargs: Any) -> JsonValue:
    return _DEFAULT.calc_eot(ephem, jd_utc, lon_deg, **kwargs)


def ganzhi(ephem: str, date: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.ganzhi(ephem, date, **kwargs)


def ganzhi_month(ephem: str, year: int, month: int, **kwargs: Any) -> JsonValue:
    return _DEFAULT.ganzhi_month(ephem, year, month, **kwargs)


def monthview(ephem: str, year_month: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.monthview(ephem, year_month, **kwargs)


def export_days(ephem: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.export_days(ephem, **kwargs)


def at(ephem: str, time: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.at(ephem, time, **kwargs)


def convert(ephem: str, value: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.convert(ephem, value, **kwargs)


def from_lunar(ephem: str, lunar_year: int, month_no: int, lunar_day: int, **kwargs: Any) -> JsonValue:
    return _DEFAULT.from_lunar(ephem, lunar_year, month_no, lunar_day, **kwargs)


def next_events(ephem: str, from_time: str, count: int, **kwargs: Any) -> JsonValue:
    return _DEFAULT.next_events(ephem, from_time, count, **kwargs)


def range_events(ephem: str, from_time: str, to_time: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.range_events(ephem, from_time, to_time, **kwargs)


def search(ephem: str, query: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.search(ephem, query, **kwargs)


def zodiac(ephem: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.zodiac(ephem, **kwargs)


def sky(ephem: str, time: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.sky(ephem, time, **kwargs)


def eclipse(ephem: str, near: str | None = None, **kwargs: Any) -> JsonValue:
    return _DEFAULT.eclipse(ephem, near, **kwargs)


def festival(ephem: str, year: int, **kwargs: Any) -> JsonValue:
    return _DEFAULT.festival(ephem, year, **kwargs)


def almanac(ephem: str, date: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.almanac(ephem, date, **kwargs)


def info(ephem: str, **kwargs: Any) -> JsonValue:
    return _DEFAULT.info(ephem, **kwargs)
