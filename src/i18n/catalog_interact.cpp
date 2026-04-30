#include "lunar/i18n_interact.hpp"

#include<string>

#include "lunar/i18n.hpp"

namespace lunar::i18n::interact{

namespace{

struct Item{
	const char*key;
	const char*zh;
	const char*en;
	const char*ja;
	const char*ko;
	const char*zht=nullptr;
};

const Item kItems[]={{"done_back",
									 "已完成，按回车返回主菜单。",
									 "Done. Press Enter to return to the main menu.",
									 "完了しました。Enter キーでメインメニューに戻ります。",
									 "완료되었습니다. Enter 키로 메인 메뉴로 돌아갑니다。"},
									{"back",
									 "按回车返回主菜单。",
									 "Press Enter to return to the main menu.",
									 "Enter キーでメインメニューに戻ります。",
									 "Enter 키로 메인 메뉴로 돌아갑니다。"},
									{"retry_select_ephem",
									 "未成功选择星历文件，是否重试？",
									 "No ephemeris was selected successfully. Retry?",
									 "星暦を選択できませんでした。再試行しますか？",
									 "성공적으로 성력을 선택하지 못했습니다. 다시 시도할까요?"},
									{"retry_prompt",
									 "继续重试？",
									 "Retry?",
									 "再試行しますか？",
									 "다시 시도할까요?"},
									{"no_ephem_exit",
									 "未选择星历文件，程序退出。",
									 "No ephemeris selected. Exiting.",
									 "星暦が選択されていないため終了します。",
									 "성력을 선택하지 않아 종료합니다."},
									{"current_ephem",
									 "当前星历文件：{0}",
									 "Current ephemeris: {0}",
									 "現在の星暦ファイル: {0}",
									 "현재 성력 파일: {0}"},
									{"choose_action",
									 "请选择要执行的操作：",
									 "Choose an action:",
									 "実行する操作を選択してください:",
									 "실행할 작업을 선택하세요:"},
									{"input_select",
									 "请输入选项：",
									 "Select:",
									 "選択してください:",
									 "선택하세요:"},
									{"invalid_option",
									 "无效的选项，请重新输入。",
									 "Invalid option. Please try again.",
									 "無効な選択です。再入力してください。",
									 "잘못된 선택입니다. 다시 입력하세요."},
									{"exit",
									 "感谢使用，程序即将退出。",
									 "Thanks for using lunar. Exiting.",
									 "ご利用ありがとうございました。終了します。",
									 "이용해 주셔서 감사합니다. 종료합니다."},
									{"menu.months",
									 "计算农历 / 公历月份信息",
									 "Month data (lunar/gregorian)",
									 "月情報（旧暦/西暦）",
									 "월 정보(음력/양력)"},
									{"menu.calendar",
									 "计算整年节气与月相日历",
									 "Full-year calendar (solar terms/lunar phases)",
									 "年カレンダー（節気/月相）",
									 "연간 달력(절기/월상)"},
									{"menu.at",
									 "任意时刻查询（月相/节气/农历）",
									 "Query a specific moment (phase/term/lunar date)",
									 "任意時刻照会（月相/節気/旧暦）",
									 "임의 시각 조회(월상/절기/음력)"},
									{"menu.convert",
									 "公历转农历 / 农历转公历",
									 "Gregorian <-> lunar conversion",
									 "西暦<->旧暦 変換",
									 "양력<->음력 변환"},
									{"menu.zodiac",
									 "太阳星座查询",
									 "Solar zodiac lookup",
									 "太陽星座照会",
									 "태양 별자리"},
									{"menu.day","日历单日视图","Day view","日ビュー","일 뷰"},
									{"menu.next","后续事件查询","Upcoming events","次イベント検索","다음 이벤트"},
									{"menu.festival","传统节日","Traditional festivals","伝統祝日","전통 명절"},
									{"menu.info","星历信息","Ephemeris info","星暦情報","성력 정보"},
									{"menu.monthview","月历视图（月）","Month view","月ビュー","월 뷰"},
									{"menu.range","区间事件查询","Events in range","区間イベント検索","구간 이벤트"},
									{"menu.search","自然语言事件检索","Natural-language event search","自然文検索","검색"},
									{"menu.eclipse","日月食查询","Eclipse lookup","食検索","식 조회"},
									{"menu.almanac","黄历/宜忌","Almanac","黄暦","황력"},
									{"menu.config","配置查看/修改","Show / set config","設定表示/変更","설정 보기/수정"},
									{"menu.completion","补全脚本生成","Generate completion script","補完スクリプト生成","완성 스크립트"},
									{"menu.sky","观星位置查询","Sky-position lookup","天体位置照会","천체 위치 조회"},
									{"menu.export","批量导出日数据","Batch day export","日データ一括出力","일 데이터 일괄 내보내기"},
									{"menu.switch_bsp",
									 "重新选择 / 下载 BSP 星历文件",
									 "Select / download BSP ephemeris",
									 "BSP を再選択/ダウンロード",
									 "BSP 선택/다운로드"},
									{"menu.help","查看命令行使用帮助","Show CLI help","CLI ヘルプ表示","CLI 도움말"},
									{"menu.exit","退出程序","Exit","終了","종료"},
									{"err.cmd_failed","运行 {0} 时出错：","{0} failed: ","{0} 実行エラー: ","{0} 실행 오류: "},
									{"prompt.out_file",
									 "可选：输出文件路径（留空则输出到控制台）：",
									 "Optional: output file path (empty for console): ",
									 "任意: 出力ファイルパス（空でコンソール出力）: ",
									 "선택: 출력 파일 경로(비우면 콘솔): "},
									{"prompt.format_txt_json",
									 "输出格式：1) txt 2) json（默认 txt）：",
									 "Output format: 1) txt 2) json (default txt): ",
									 "出力形式: 1) txt 2) json（既定 txt）: ",
									 "출력 형식: 1) txt 2) json (기본 txt): "},
									{"prompt.format_txt_json_csv",
									 "输出格式：1) txt 2) json 3) csv（默认 txt）：",
									 "Output format: 1) txt 2) json 3) csv (default txt): ",
									 "出力形式: 1) txt 2) json 3) csv（既定 txt）: ",
									 "출력 형식: 1) txt 2) json 3) csv (기본 txt): "},
									{"prompt.format_event",
									 "输出格式：1) txt 2) json 3) csv 4) ics 5) jsonl（默认 txt）：",
									 "Output format: 1) txt 2) json 3) csv 4) ics 5) jsonl (default txt): ",
									 "出力形式: 1) txt 2) json 3) csv 4) ics 5) jsonl（既定 txt）: ",
									 "출력 형식: 1) txt 2) json 3) csv 4) ics 5) jsonl (기본 txt): "},
									{"prompt.format_eclipse",
									 "输出格式：1) json 2) txt 3) geojson（默认 json）：",
									 "Output format: 1) json 2) txt 3) geojson (default json): ",
									 "出力形式: 1) json 2) txt 3) geojson（既定 json）: ",
									 "출력 형식: 1) json 2) txt 3) geojson (기본 json): "},
									{"prompt.zodiac_mode",
									 "星座查询模式：1) 单点时刻 2) 全年持续时间（默认 1）：",
									 "Zodiac mode: 1) point time 2) whole year duration (default 1): ",
									 "星座照会モード: 1) 単一点時刻 2) 年間継続時間（既定 1）: ",
									 "별자리 모드: 1) 단일 시각 2) 연간 지속시간 (기본 1): "},
									{"prompt.zodiac_time",
									 "请输入查询时刻（例如 2025-03-20T18:01:00+08:00）：",
									 "Enter query time (e.g. 2025-03-20T18:01:00+08:00): ",
									 "照会時刻を入力（例: 2025-03-20T18:01:00+08:00）: ",
									 "조회 시각 입력(예: 2025-03-20T18:01:00+08:00): "},
									{"prompt.zodiac_input_tz",
									 "若输入不带时区，解析时区（默认 +08:00）：",
									 "Input timezone when time has no TZ (default +08:00): ",
									 "入力値にタイムゾーンが無い場合の解釈 TZ（既定 +08:00）: ",
									 "입력값에 시간대가 없을 때 해석 TZ (기본 +08:00): "},
									{"prompt.zodiac_display_tz",
									 "输出显示时区（默认 +08:00；全年模式也用于年界裁剪）：",
									 "Display timezone (default +08:00; also used to clip the civil year in year mode): ",
									 "表示タイムゾーン（既定 +08:00、年間モードでは年境界切り出しにも使用）: ",
									 "표시 시간대(기본 +08:00, 연간 모드에서는 연도 절단에도 사용): "},
									{"prompt.zodiac_year",
									 "请输入公历年份（例如 2025）：",
									 "Enter Gregorian year (e.g. 2025): ",
									 "西暦年を入力（例: 2025）: ",
									 "양력 연도 입력(예: 2025): "},
									{"prompt.sky_time",
									 "请输入观测时刻（例如 2025-06-01T20:00:00+08:00）：",
									 "Enter observation time (e.g. 2025-06-01T20:00:00+08:00): ",
									 "観測時刻を入力（例: 2025-06-01T20:00:00+08:00）: ",
									 "관측 시각을 입력하세요(예: 2025-06-01T20:00:00+08:00): "},
									{"prompt.sky_input_tz",
									 "若输入不带时区，解析时区（默认 +08:00）：",
									 "Input timezone when time has no TZ (default +08:00): ",
									 "入力値にタイムゾーンが無い場合の解釈 TZ（既定 +08:00）: ",
									 "입력값에 시간대가 없을 때 해석할 TZ(기본 +08:00): "},
									{"prompt.sky_display_tz",
									 "输出显示时区（默认 +08:00）：",
									 "Display timezone (default +08:00): ",
									 "表示タイムゾーン（既定 +08:00）: ",
									 "표시 시간대(기본 +08:00): "},
									{"prompt.sky_lat",
									 "请输入观测点纬度（度）：",
									 "Enter observer latitude in degrees: ",
									 "観測地の緯度（度）を入力: ",
									 "관측 지점 위도(도)를 입력하세요: "},
									{"prompt.sky_lon",
									 "请输入观测点经度（度，东经为正）：",
									 "Enter observer longitude in degrees (east positive): ",
									 "観測地の経度（度、東経正）を入力: ",
									 "관측 지점 경도(도, 동경 양수)를 입력하세요: "},
									{"prompt.sky_height",
									 "可选：观测点海拔米数（默认 0）：",
									 "Optional: observer height in meters (default 0): ",
									 "任意: 観測地の標高メートル（既定 0）: ",
									 "선택: 관측 지점 해발(m, 기본 0): "},
									{"prompt.sky_mode",
									 "查询模式：1) all 2) pick（默认 all）：",
									 "Query mode: 1) all 2) pick (default all): ",
									 "照会モード: 1) all 2) pick（既定 all）: ",
									 "조회 모드: 1) all 2) pick (기본 all): "},
									{"prompt.sky_pick",
									 "请输入目标列表（逗号分隔，例如 sun,moon,Spica）：",
									 "Enter targets (comma separated, e.g. sun,moon,Spica): ",
									 "対象一覧を入力（カンマ区切り、例: sun,moon,Spica）: ",
									 "대상 목록을 입력하세요(쉼표 구분, 예: sun,moon,Spica): "},
									{"prompt.monthview_ym",
									 "请输入年月 YYYY-MM：",
									 "Enter year-month YYYY-MM: ",
									 "年月 YYYY-MM を入力: ",
									 "연월 YYYY-MM 입력: "},
									{"prompt.range_from",
									 "请输入起始时刻（例如 2025-01-01T00:00:00+08:00）：",
									 "Enter start time (e.g. 2025-01-01T00:00:00+08:00): ",
									 "開始時刻を入力（例: 2025-01-01T00:00:00+08:00）: ",
									 "시작 시각 입력(예: 2025-01-01T00:00:00+08:00): "},
									{"prompt.range_to",
									 "请输入结束时刻（例如 2025-12-31T23:59:59+08:00）：",
									 "Enter end time (e.g. 2025-12-31T23:59:59+08:00): ",
									 "終了時刻を入力（例: 2025-12-31T23:59:59+08:00）: ",
									 "종료 시각 입력(예: 2025-12-31T23:59:59+08:00): "},
									{"prompt.kinds_optional",
									 "可选：事件类型（逗号分隔，留空为全部）：",
									 "Optional: kinds list (comma separated, empty for all): ",
									 "任意: イベント種別（カンマ区切り、空で全部）: ",
									 "선택: 이벤트 종류(쉼표 구분, 비우면 전체): "},
									{"prompt.search_query",
									 "请输入检索语句（例如 next full moon）：",
									 "Enter query (e.g. next full moon): ",
									 "検索文を入力（例: next full moon）: ",
									 "검색어 입력(예: next full moon): "},
									{"prompt.search_from",
									 "可选：--from 时刻（留空使用默认）：",
									 "Optional: --from time (empty for default): ",
									 "任意: --from 時刻（空で既定値）: ",
									 "선택: --from 시각(비우면 기본값): "},
									{"prompt.search_count",
									 "可选：数量 --count（默认 1）：",
									 "Optional: --count (default 1): ",
									 "任意: 件数 --count（既定 1）: ",
									 "선택: 개수 --count (기본 1): "},
									{"prompt.eclipse_near",
									 "请输入邻近日期 YYYY-MM-DD：",
									 "Enter nearby date YYYY-MM-DD: ",
									 "近傍日付 YYYY-MM-DD を入力: ",
									 "근접 날짜 YYYY-MM-DD 입력: "},
									{"prompt.eclipse_kind",
									 "食类型：1) lunar 2) solar（默认 lunar）：",
									 "Eclipse kind: 1) lunar 2) solar (default lunar): ",
									 "食タイプ: 1) lunar 2) solar（既定 lunar）: ",
									 "식 종류: 1) lunar 2) solar (기본 lunar): "},
									{"prompt.eclipse_stage",
									 "可选：阶段窗口 --stage（留空 any）：",
									 "Optional: --stage window (leave empty for any): ",
									 "任意: --stage（空で any）: ",
									 "선택: --stage (비우면 any): "},
									{"prompt.eclipse_global",
									 "是否计算全局可见性：1)是 0)否（默认0）：",
									 "Compute global visibility? 1) yes 0) no (default 0): ",
									 "全球可視性を計算しますか？ 1)はい 0)いいえ（既定0）: ",
									 "전역 가시성 계산? 1)예 0)아니오 (기본 0): "},
									{"prompt.almanac_date",
									 "请输入公历日期 YYYY-MM-DD：",
									 "Enter Gregorian date YYYY-MM-DD: ",
									 "西暦日付 YYYY-MM-DD を入力: ",
									 "양력 날짜 YYYY-MM-DD 입력: "},
									{"prompt.config_action",
									 "配置操作：1) show 2) set（默认 show）：",
									 "Config action: 1) show 2) set (default show): ",
									 "設定操作: 1) show 2) set（既定 show）: ",
									 "설정 작업: 1) show 2) set (기본 show): "},
									{"prompt.config_key",
									 "请输入配置键（def_bsp|bsp_dir|bsp_list|default_tz|default_lang|default_lunar_day_tz|def_fmt|hli_trad|hli_year_boundary|hli_month_boundary|hli_leap_month_mode|hli_day_boundary|def_prety）：",
									 "Enter config key (def_bsp|bsp_dir|bsp_list|default_tz|default_lang|default_lunar_day_tz|def_fmt|hli_trad|hli_year_boundary|hli_month_boundary|hli_leap_month_mode|hli_day_boundary|def_prety): ",
									 "設定キーを入力（def_bsp|bsp_dir|bsp_list|default_tz|default_lang|default_lunar_day_tz|def_fmt|hli_trad|hli_year_boundary|hli_month_boundary|hli_leap_month_mode|hli_day_boundary|def_prety）: ",
									 "설정 키 입력(def_bsp|bsp_dir|bsp_list|default_tz|default_lang|default_lunar_day_tz|def_fmt|hli_trad|hli_year_boundary|hli_month_boundary|hli_leap_month_mode|hli_day_boundary|def_prety): "},
									{"prompt.config_value",
									 "请输入配置值：",
									 "Enter config value: ",
									 "設定値を入力: ",
									 "설정 값 입력: "},
									{"prompt.comp_shell",
									 "补全脚本类型：1) bash 2) zsh 3) fish 4) powershell（默认 powershell）：",
									 "Completion shell: 1) bash 2) zsh 3) fish 4) powershell (default powershell): ",
									 "補完シェル: 1) bash 2) zsh 3) fish 4) powershell（既定 powershell）: ",
									 "완성 셸: 1) bash 2) zsh 3) fish 4) powershell (기본 powershell): "},
									{"menu.lang","切换界面语言","Change language","表示言語を切り替え","언어 변경"},
									{"lang.current","当前界面语言：{0}","Current UI language: {0}","現在の表示言語: {0}","현재 UI 언어: {0}"},
									{"prompt.lang_choice",
									 "选择语言：1) 简体中文 2) 繁體中文 3) English 4) 日本語 5) 한국어（也可直接输入 zh|zht|en|ja|ko，回车返回）：",
									 "Choose language: 1) Simplified Chinese 2) Traditional Chinese 3) English 4) Japanese 5) Korean (or enter zh|zht|en|ja|ko, Enter to go back): ",
									 "言語を選択: 1) 簡体中文 2) 繁體中文 3) English 4) 日本語 5) 한국어（zh|zht|en|ja|ko を直接入力可、Enter で戻る）: ",
									 "언어 선택: 1) 간체 중국어 2) 번체 중국어 3) English 4) 日本語 5) 한국어 (zh|zht|en|ja|ko 직접 입력 가능, Enter로 돌아감): "},
									{"lang.updated",
									 "界面语言已切换为 {0}，并已写入 default_lang。",
									 "UI language switched to {0} and saved to default_lang.",
									 "表示言語を {0} に切り替え、default_lang に保存しました。",
									 "UI 언어를 {0}(으)로 변경했고 default_lang에 저장했습니다."},
									{"lang.invalid",
									 "无效的语言选项，请输入 1-5 或 zh|zht|en|ja|ko。",
									 "Invalid language choice. Enter 1-5 or zh|zht|en|ja|ko.",
									 "無効な言語選択です。1-5 または zh|zht|en|ja|ko を入力してください。",
									 "잘못된 언어 선택입니다. 1-5 또는 zh|zht|en|ja|ko 를 입력하세요."},
									{"info.no_bsp_found",
									 "未找到可用 BSP 文件，下面可直接下载。",
									 "No usable BSP files found. You can download one below.",
									 "利用可能な BSP ファイルが見つかりません。以下からダウンロードできます。",
									 "사용 가능한 BSP 파일을 찾지 못했습니다. 아래에서 바로 다운로드할 수 있습니다."},
									{"prompt.use_series_fallback",
									 "未下载 BSP。是否改用内置 VSOP87A/ELPMPP02 星历？",
									 "No BSP downloaded. Use the built-in VSOP87A/ELPMPP02 ephemeris instead?",
									 "BSP をダウンロードしていません。内蔵 VSOP87A/ELPMPP02 星暦を使用しますか？",
									 "BSP를 다운로드하지 않았습니다. 내장 VSOP87A/ELPMPP02 천체력을 대신 사용할까요?"},
									{"info.series_selected",
									 "已切换到内置 VSOP87A/ELPMPP02 星历。",
									 "Switched to the built-in VSOP87A/ELPMPP02 ephemeris.",
									 "内蔵 VSOP87A/ELPMPP02 星暦へ切り替えました。",
									 "내장 VSOP87A/ELPMPP02 천체력으로 전환했습니다."},
									{"err.empty_required",
									 "{0}不能为空",
									 "{0} must not be empty",
									 "{0} は必須です",
									 "{0} 은(는) 필수입니다"}};

const Item*find_item(const std::string&key){
	for(const auto&item : kItems){
		if(key==item.key){
			return &item;
		}
	}
	return nullptr;
}

std::string pick_item(const Item&item){
	switch(current_lang()){
		case Lang::Zh:
			return item.zh;
		case Lang::ZhHant:
			if(item.zht&&item.zht[0]!='\0'){
				return item.zht;
			}
			return to_zh_hant(item.zh);
		case Lang::En:
			return item.en;
		case Lang::Ja:
			return item.ja;
		case Lang::Ko:
			return item.ko;
	}
	return item.zh;
}

void replace_all(std::string&text,const std::string&needle,const std::string&value){
	std::size_t pos=0;
	while((pos=text.find(needle,pos))!=std::string::npos){
		text.replace(pos,needle.size(),value);
		pos+=value.size();
	}
}

std::string base_text(const std::string&key){
	const Item*item=find_item(key);
	if(item==nullptr){
		return key;
	}
	return pick_item(*item);
}

}

std::string text(const std::string&key){ return base_text(key); }

std::string textf(const std::string&key,const std::string&a0){
	std::string out=base_text(key);
	replace_all(out,"{0}",a0);
	return out;
}

std::string textf(const std::string&key,const std::string&a0,const std::string&a1){
	std::string out=base_text(key);
	replace_all(out,"{0}",a0);
	replace_all(out,"{1}",a1);
	return out;
}

}



