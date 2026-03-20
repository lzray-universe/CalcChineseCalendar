#pragma once

#include<array>
#include<cstddef>
#include<cstdint>

namespace elpmpp02{

enum class CorrectionSet : int{
	LLR=0,
	DE405=1,
	DE406=2
};

enum class Coordinate : int{
	Longitude=0,
	Latitude=1,
	Distance=2,
};

struct MainTermRaw{
	std::array<std::int8_t,4> ilu;
	double a;
	std::array<double,5> b;
};

struct PertTermRaw{
	double s;
	double c;
	std::array<std::int8_t,16> ifi;
};

struct TermBlockMain{
	const MainTermRaw*data;
	std::size_t count;
};

struct TermBlockPert{
	const PertTermRaw*data;
	std::size_t count;
};

struct RawSeriesData{
	std::array<TermBlockMain,3> main;
	std::array<std::array<TermBlockPert,4>,3> pert;
};

struct StateVector{
	std::array<double,3> position_km{};
	std::array<double,3> velocity_km_per_day{};
};

const RawSeriesData&GetRawSeriesData();

void Evaluate(CorrectionSet correction,double jd,StateVector&out);

void EvaluateFromJ2000Days(CorrectionSet correction,double tj_days,
						   StateVector&out);

constexpr std::size_t kMainLongitudeCount=1023;
constexpr std::size_t kMainLatitudeCount=918;
constexpr std::size_t kMainDistanceCount=704;

constexpr std::size_t kPertLongitudeCounts[4]={11314,1199,219,2};
constexpr std::size_t kPertLatitudeCounts[4]={6462,516,52,0};
constexpr std::size_t kPertDistanceCounts[4]={12115,1165,210,2};

}
