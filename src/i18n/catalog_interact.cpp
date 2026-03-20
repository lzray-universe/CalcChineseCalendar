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
									 "No ephemeris selected successfully. Retry?",
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
									 "Months (lunar/gregorian)",
									 "月情報（旧暦/西暦）",
									 "월 정보(음력/양력)"},
									{"menu.calendar",
									 "计算整年节气与月相日历",
									 "Year calendar (terms/phases)",
									 "年カレンダー（節気/月相）",
									 "연간 달력(절기/월상)"},
									{"menu.at",
									 "任意时刻查询（月相/节气/农历）",
									 "Instant query (phase/term/lunar)",
									 "任意時刻照会（月相/節気/旧暦）",
									 "임의 시각 조회(월상/절기/음력)"},
									{"menu.convert",
									 "公历转农历 / 农历转公历",
									 "Gregorian <-> Lunar convert",
									 "西暦<->旧暦 変換",
									 "양력<->음력 변환"},
									{"menu.day","日历单日视图","Day view","日ビュー","일 뷰"},
									{"menu.next","后续事件查询","Next events","次イベント検索","다음 이벤트"},
									{"menu.festival","传统节日","Traditional festivals","伝統祝日","전통 명절"},
									{"menu.info","星历信息","Ephemeris info","星暦情報","성력 정보"},
									{"menu.monthview","月历视图（月）","Month view","月ビュー","월 뷰"},
									{"menu.range","区间事件查询","Range events","区間イベント検索","구간 이벤트"},
									{"menu.search","自然语言事件检索","Search events","自然文検索","검색"},
									{"menu.eclipse","日月食查询","Eclipse query","食検索","식 조회"},
									{"menu.almanac","黄历/宜忌","Almanac","黄暦","황력"},
									{"menu.config","配置查看/修改","Config show/set","設定表示/変更","설정 보기/수정"},
									{"menu.completion","补全脚本生成","Completion script","補完スクリプト生成","완성 스크립트"},
									{"menu.switch_bsp",
									 "重新选择 / 下载 BSP 星历文件",
									 "Select/Download BSP",
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
									 "请输入检索语句（例如 next full_moon）：",
									 "Enter query (e.g. next full_moon): ",
									 "検索文を入力（例: next full_moon）: ",
									 "검색어 입력(예: next full_moon): "},
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
									 "Optional: --stage window (empty as any): ",
									 "任意: --stage（空で any）: ",
									 "선택: --stage (비우면 any): "},
									{"prompt.eclipse_global",
									 "是否计算全局可见性：1)是 0)否（默认0）：",
									 "Compute global visibility? 1)yes 0)no (default 0): ",
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
									 "请输入配置键（def_bsp|bsp_dir|bsp_list|default_tz|default_lang|def_fmt|hli_trad|hli_year_boundary|hli_month_boundary|hli_leap_month_mode|hli_day_boundary|def_prety）：",
									 "Enter config key (def_bsp|bsp_dir|bsp_list|default_tz|default_lang|def_fmt|hli_trad|hli_year_boundary|hli_month_boundary|hli_leap_month_mode|hli_day_boundary|def_prety): ",
									 "設定キーを入力（def_bsp|bsp_dir|bsp_list|default_tz|default_lang|def_fmt|hli_trad|hli_year_boundary|hli_month_boundary|hli_leap_month_mode|hli_day_boundary|def_prety）: ",
									 "설정 키 입력(def_bsp|bsp_dir|bsp_list|default_tz|default_lang|def_fmt|hli_trad|hli_year_boundary|hli_month_boundary|hli_leap_month_mode|hli_day_boundary|def_prety): "},
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
									{"err.empty_required",
									 "{0}不能为空",
									 "{0} cannot be empty",
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



