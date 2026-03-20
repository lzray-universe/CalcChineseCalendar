#include "elpmpp02.hpp"
#include "elpmpp02_data_decl.hpp"

#include<array>
#include<cmath>
#include<cstddef>
#include<cstdint>
#include<mutex>
#include<vector>

namespace elpmpp02{
namespace{

constexpr double kPi=3.141592653589793238462643383279502884;
constexpr double kDegToRad=kPi/180.0;
constexpr double kRad=648000.0/kPi;
constexpr double kDj2000=2451545.0;
constexpr double kA405=384747.9613701725;
constexpr double kAelp=384747.980674318;
constexpr double kSc=36525.0;
constexpr double kPis2=kPi/2.0;
constexpr double kDpi=2.0*kPi;

inline double Dms(int ideg,int imin,double sec){
	return (static_cast<double>(ideg)+static_cast<double>(imin)/60.0+
			sec/3600.0)*
		   kDegToRad;
}

struct Constants{
	double w[3][5]{};
	double eart[5]{};
	double peri[5]{};
	double zeta[5]{};
	double del[4][5]{};
	double p[8][5]{};
	double delnu{};
	double dele{};
	double delg{};
	double delnp{};
	double delep{};
	double dtasm{};
	double am{};
	double p1{},p2{},p3{},p4{},p5{};
	double q1{},q2{},q3{},q4{},q5{};
};

struct PreparedMainTerm{
	double coeff{};
	std::array<double,5> phase{};
};

struct PreparedPertTerm{
	double amp{};
	int power{};
	std::array<double,5> phase{};
};

struct CachedSeries{
	Constants c;
	std::array<std::vector<PreparedMainTerm>,3> main;
	std::array<std::array<std::vector<PreparedPertTerm>,4>,3> pert;
};

CorrectionSet Normalize(CorrectionSet c){
	if(c==CorrectionSet::DE406){
		return CorrectionSet::DE405;
	}
	return c;
}

Constants BuildConstants(CorrectionSet correction){
	correction=Normalize(correction);
	Constants c{};

	const double Dprec=-0.29965;
	c.am=0.074801329;
	const double alpha=0.002571881;
	c.dtasm=(2.0*alpha)/(3.0*c.am);
	const double xa=(2.0*alpha)/3.0;

	const double bp[5][2]={
		{+0.311079095e+00,-0.103837907e+00},{-0.4482398e-2,+0.6682870e-3},
		{-0.110248500e-2,-0.129807200e-2},	{+0.1056062e-2,-0.1780280e-3},
		{+0.50928e-4,-0.37342e-4},
	};

	double Dw1_0,Dw2_0,Dw3_0,Deart_0,Dperi,Dw1_1,Dgam,De,Deart_1,Dep,Dw2_1,
		Dw3_1,Dw1_2;
	if(correction==CorrectionSet::LLR){
		Dw1_0=-0.10525;
		Dw2_0=0.16826;
		Dw3_0=-0.10760;
		Deart_0=-0.04012;
		Dperi=-0.04854;
		Dw1_1=-0.32311;
		Dgam=0.00069;
		De=+0.00005;
		Deart_1=0.01442;
		Dep=0.00226;
		Dw2_1=0.08017;
		Dw3_1=-0.04317;
		Dw1_2=-0.03794;
	}else{
		Dw1_0=-0.07008;
		Dw2_0=0.20794;
		Dw3_0=-0.07215;
		Deart_0=-0.00033;
		Dperi=-0.00749;
		Dw1_1=-0.35106;
		Dgam=0.00085;
		De=-0.00006;
		Deart_1=0.00732;
		Dep=0.00224;
		Dw2_1=0.08017;
		Dw3_1=-0.04317;
		Dw1_2=-0.03743;
	}

	c.w[0][0]=Dms(218,18,59.95571+Dw1_0);
	c.w[0][1]=(1732559343.73604+Dw1_1)/kRad;
	c.w[0][2]=(-6.8084+Dw1_2)/kRad;
	c.w[0][3]=0.66040e-2/kRad;
	c.w[0][4]=-0.31690e-4/kRad;

	c.w[1][0]=Dms(83,21,11.67475+Dw2_0);
	c.w[1][1]=(14643420.3171+Dw2_1)/kRad;
	c.w[1][2]=(-38.2631)/kRad;
	c.w[1][3]=-0.45047e-1/kRad;
	c.w[1][4]=0.21301e-3/kRad;

	c.w[2][0]=Dms(125,2,40.39816+Dw3_0);
	c.w[2][1]=(-6967919.5383+Dw3_1)/kRad;
	c.w[2][2]=(6.3590)/kRad;
	c.w[2][3]=0.76250e-2/kRad;
	c.w[2][4]=-0.35860e-4/kRad;

	c.eart[0]=Dms(100,27,59.13885+Deart_0);
	c.eart[1]=(129597742.29300+Deart_1)/kRad;
	c.eart[2]=-0.020200/kRad;
	c.eart[3]=0.90000e-5/kRad;
	c.eart[4]=0.15000e-6/kRad;

	c.peri[0]=Dms(102,56,14.45766+Dperi);
	c.peri[1]=1161.24342/kRad;
	c.peri[2]=0.529265/kRad;
	c.peri[3]=-0.11814e-3/kRad;
	c.peri[4]=0.11379e-4/kRad;

	if(correction==CorrectionSet::DE405){
		c.w[0][3]-=0.00018865/kRad;
		c.w[0][4]-=0.00001024/kRad;
		c.w[1][2]+=0.00470602/kRad;
		c.w[1][3]-=0.00025213/kRad;
		c.w[2][2]-=0.00261070/kRad;
		c.w[2][3]-=0.00010712/kRad;
	}

	const double x2=c.w[1][1]/c.w[0][1];
	const double x3=c.w[2][1]/c.w[0][1];
	const double y2=c.am*bp[0][0]+xa*bp[4][0];
	const double y3=c.am*bp[0][1]+xa*bp[4][1];

	const double d21=x2-y2;
	const double d22=c.w[0][1]*bp[1][0];
	const double d23=c.w[0][1]*bp[2][0];
	const double d24=c.w[0][1]*bp[3][0];
	const double d25=y2/c.am;

	const double d31=x3-y3;
	const double d32=c.w[0][1]*bp[1][1];
	const double d33=c.w[0][1]*bp[2][1];
	const double d34=c.w[0][1]*bp[3][1];
	const double d35=y3/c.am;

	const double Cw2_1=d21*Dw1_1+d25*Deart_1+d22*Dgam+d23*De+d24*Dep;
	const double Cw3_1=d31*Dw1_1+d35*Deart_1+d32*Dgam+d33*De+d34*Dep;

	c.w[1][1]+=Cw2_1/kRad;
	c.w[2][1]+=Cw3_1/kRad;

	for(int i=0;i<=4;++i){
		c.del[0][i]=c.w[0][i]-c.eart[i];
		c.del[1][i]=c.w[0][i]-c.w[2][i];
		c.del[2][i]=c.w[0][i]-c.w[1][i];
		c.del[3][i]=c.eart[i]-c.peri[i];
	}
	c.del[0][0]+=kPi;

	c.p[0][0]=Dms(252,15,3.216919);
	c.p[1][0]=Dms(181,58,44.758419);
	c.p[2][0]=Dms(100,27,59.138850);
	c.p[3][0]=Dms(355,26,3.642778);
	c.p[4][0]=Dms(34,21,5.379392);
	c.p[5][0]=Dms(50,4,38.902495);
	c.p[6][0]=Dms(314,3,4.354234);
	c.p[7][0]=Dms(304,20,56.808371);

	c.p[0][1]=538101628.66888/kRad;
	c.p[1][1]=210664136.45777/kRad;
	c.p[2][1]=129597742.29300/kRad;
	c.p[3][1]=68905077.65936/kRad;
	c.p[4][1]=10925660.57335/kRad;
	c.p[5][1]=4399609.33632/kRad;
	c.p[6][1]=1542482.57845/kRad;
	c.p[7][1]=786547.89700/kRad;

	c.zeta[0]=c.w[0][0];
	c.zeta[1]=c.w[0][1]+(5029.0966+Dprec)/kRad;
	c.zeta[2]=c.w[0][2];
	c.zeta[3]=c.w[0][3];
	c.zeta[4]=c.w[0][4];

	c.delnu=(+0.55604+Dw1_1)/kRad/c.w[0][1];
	c.dele=(+0.01789+De)/kRad;
	c.delg=(-0.08066+Dgam)/kRad;
	c.delnp=(-0.06424+Deart_1)/kRad/c.w[0][1];
	c.delep=(-0.12879+Dep)/kRad;

	c.p1=0.10180391e-04;
	c.p2=0.47020439e-06;
	c.p3=-0.5417367e-09;
	c.p4=-0.2507948e-11;
	c.p5=0.463486e-14;

	c.q1=-0.113469002e-03;
	c.q2=0.12372674e-06;
	c.q3=0.1265417e-08;
	c.q4=-0.1371808e-11;
	c.q5=-0.320334e-14;

	return c;
}

CachedSeries BuildCache(CorrectionSet correction){
	correction=Normalize(correction);
	CachedSeries cache{};
	cache.c=BuildConstants(correction);
	const auto&raw=GetRawSeriesData();

	for(int iv=0;iv<3;++iv){
		cache.main[iv].reserve(raw.main[iv].count);
		for(std::size_t n=0;n<raw.main[iv].count;++n){
			const auto&rt=raw.main[iv].data[n];
			PreparedMainTerm pt{};
			const double tgv=rt.b[0]+cache.c.dtasm*rt.b[4];
			double a=rt.a;
			if(iv==2){
				a=a-2.0*a*cache.c.delnu/3.0;
			}
			pt.coeff=a+tgv*(cache.c.delnp-cache.c.am*cache.c.delnu)+
					 rt.b[1]*cache.c.delg+rt.b[2]*cache.c.dele+
					 rt.b[3]*cache.c.delep;
			for(int k=0;k<=4;++k){
				pt.phase[k]=0.0;
				for(int i=0;i<4;++i){
					pt.phase[k]+=
						static_cast<double>(rt.ilu[i])*cache.c.del[i][k];
				}
			}
			if(iv==2){
				pt.phase[0]+=kPis2;
			}
			cache.main[iv].push_back(pt);
		}

		for(int it=0;it<4;++it){
			cache.pert[iv][it].reserve(raw.pert[iv][it].count);
			for(std::size_t n=0;n<raw.pert[iv][it].count;++n){
				const auto&rt=raw.pert[iv][it].data[n];
				PreparedPertTerm pt{};
				pt.power=it;
				pt.amp=std::sqrt(rt.c*rt.c+rt.s*rt.s);
				double pha=std::atan2(rt.c,rt.s);
				if(pha<0.0){
					pha+=kDpi;
				}
				pt.phase[0]=pha;
				for(int k=0;k<=4;++k){
					if(k!=0){
						pt.phase[k]=0.0;
					}
					for(int i=0;i<4;++i){
						pt.phase[k]+=
							static_cast<double>(rt.ifi[i])*cache.c.del[i][k];
					}
					for(int i=4;i<12;++i){
						pt.phase[k]+=
							static_cast<double>(rt.ifi[i])*cache.c.p[i-4][k];
					}
					pt.phase[k]+=
						static_cast<double>(rt.ifi[12])*cache.c.zeta[k];
				}
				cache.pert[iv][it].push_back(pt);
			}
		}
	}

	return cache;
}

const CachedSeries&GetCache(CorrectionSet correction){
	correction=Normalize(correction);
	static std::once_flag llr_once;
	static std::once_flag de405_once;
	static CachedSeries llr_cache;
	static CachedSeries de405_cache;

	if(correction==CorrectionSet::LLR){
		std::call_once(llr_once,
					   [](){ llr_cache=BuildCache(CorrectionSet::LLR); });
		return llr_cache;
	}
	std::call_once(de405_once,
				   [](){ de405_cache=BuildCache(CorrectionSet::DE405); });
	return de405_cache;
}

}

const RawSeriesData&GetRawSeriesData(){
	static const RawSeriesData data=[]{
		RawSeriesData d{};
		d.main[0]={detail::kMain_longitude,kMainLongitudeCount};
		d.main[1]={detail::kMain_latitude,kMainLatitudeCount};
		d.main[2]={detail::kMain_distance,kMainDistanceCount};

		d.pert[0][0]={detail::kPert_longitude_t0,kPertLongitudeCounts[0]};
		d.pert[0][1]={detail::kPert_longitude_t1,kPertLongitudeCounts[1]};
		d.pert[0][2]={detail::kPert_longitude_t2,kPertLongitudeCounts[2]};
		d.pert[0][3]={detail::kPert_longitude_t3,kPertLongitudeCounts[3]};

		d.pert[1][0]={detail::kPert_latitude_t0,kPertLatitudeCounts[0]};
		d.pert[1][1]={detail::kPert_latitude_t1,kPertLatitudeCounts[1]};
		d.pert[1][2]={detail::kPert_latitude_t2,kPertLatitudeCounts[2]};
		d.pert[1][3]={detail::kPert_latitude_t3,kPertLatitudeCounts[3]};

		d.pert[2][0]={detail::kPert_distance_t0,kPertDistanceCounts[0]};
		d.pert[2][1]={detail::kPert_distance_t1,kPertDistanceCounts[1]};
		d.pert[2][2]={detail::kPert_distance_t2,kPertDistanceCounts[2]};
		d.pert[2][3]={detail::kPert_distance_t3,kPertDistanceCounts[3]};
		return d;
	}();
	return data;
}

void Evaluate(CorrectionSet correction,double jd,StateVector&out){
	EvaluateFromJ2000Days(correction,jd-kDj2000,out);
}

void EvaluateFromJ2000Days(CorrectionSet correction,double tj,StateVector&out){
	const auto&cache=GetCache(correction);

	std::array<double,5> t{};
	t[0]=1.0;
	t[1]=tj/kSc;
	t[2]=t[1]*t[1];
	t[3]=t[2]*t[1];
	t[4]=t[3]*t[1];

	std::array<double,6> v{};
	for(int iv=0;iv<3;++iv){
		for(const auto&term : cache.main[iv]){
			double y=term.phase[0];
			double yp=0.0;
			for(int k=1;k<=4;++k){
				y+=term.phase[k]*t[k];
				yp+=static_cast<double>(k)*term.phase[k]*t[k-1];
			}
			v[iv]+=term.coeff*std::sin(y);
			v[iv+3]+=term.coeff*yp*std::cos(y);
		}

		for(int it=0;it<4;++it){
			for(const auto&term : cache.pert[iv][it]){
				double y=term.phase[0];
				double yp=0.0;
				for(int k=1;k<=4;++k){
					y+=term.phase[k]*t[k];
					yp+=static_cast<double>(k)*term.phase[k]*t[k-1];
				}
				const double xp=
					(it!=0)?static_cast<double>(it)*term.amp*t[it-1]:0.0;
				v[iv]+=term.amp*t[it]*std::sin(y);
				v[iv+3]+=xp*std::sin(y)+term.amp*t[it]*yp*std::cos(y);
			}
		}
	}

	v[0]=v[0]/kRad+cache.c.w[0][0]+cache.c.w[0][1]*t[1]+cache.c.w[0][2]*t[2]+
		 cache.c.w[0][3]*t[3]+cache.c.w[0][4]*t[4];
	v[1]=v[1]/kRad;
	v[2]=v[2]*kA405/kAelp;
	v[3]=v[3]/kRad+cache.c.w[0][1]+2.0*cache.c.w[0][2]*t[1]+
		 3.0*cache.c.w[0][3]*t[2]+4.0*cache.c.w[0][4]*t[3];
	v[4]=v[4]/kRad;

	const double clamb=std::cos(v[0]);
	const double slamb=std::sin(v[0]);
	const double cbeta=std::cos(v[1]);
	const double sbeta=std::sin(v[1]);
	const double cw=v[2]*cbeta;
	const double sw=v[2]*sbeta;

	const double x1=cw*clamb;
	const double x2=cw*slamb;
	const double x3=sw;

	const double xp1=(v[5]*cbeta-v[4]*sw)*clamb-v[3]*x2;
	const double xp2=(v[5]*cbeta-v[4]*sw)*slamb+v[3]*x1;
	const double xp3=v[5]*sbeta+v[4]*cw;

	const double pw=(cache.c.p1+cache.c.p2*t[1]+cache.c.p3*t[2]+cache.c.p4*t[3]+
					 cache.c.p5*t[4])*
					t[1];
	const double qw=(cache.c.q1+cache.c.q2*t[1]+cache.c.q3*t[2]+cache.c.q4*t[3]+
					 cache.c.q5*t[4])*
					t[1];
	const double ra=2.0*std::sqrt(1.0-pw*pw-qw*qw);
	const double pwqw=2.0*pw*qw;
	const double pw2=1.0-2.0*pw*pw;
	const double qw2=1.0-2.0*qw*qw;
	const double pwra=pw*ra;
	const double qwra=qw*ra;

	out.position_km[0]=pw2*x1+pwqw*x2+pwra*x3;
	out.position_km[1]=pwqw*x1+qw2*x2-qwra*x3;
	out.position_km[2]=-pwra*x1+qwra*x2+(pw2+qw2-1.0)*x3;

	const double ppw=cache.c.p1+(2.0*cache.c.p2+3.0*cache.c.p3*t[1]+
								 4.0*cache.c.p4*t[2]+5.0*cache.c.p5*t[3])*
									t[1];
	const double qpw=cache.c.q1+(2.0*cache.c.q2+3.0*cache.c.q3*t[1]+
								 4.0*cache.c.q4*t[2]+5.0*cache.c.q5*t[3])*
									t[1];
	const double ppw2=-4.0*pw*ppw;
	const double qpw2=-4.0*qw*qpw;
	const double ppwqpw=2.0*(ppw*qw+pw*qpw);
	const double rap=(ppw2+qpw2)/ra;
	const double ppwra=ppw*ra+pw*rap;
	const double qpwra=qpw*ra+qw*rap;

	out.velocity_km_per_day[0]=
		(pw2*xp1+pwqw*xp2+pwra*xp3+ppw2*x1+ppwqpw*x2+ppwra*x3)/kSc;
	out.velocity_km_per_day[1]=
		(pwqw*xp1+qw2*xp2-qwra*xp3+ppwqpw*x1+qpw2*x2-qpwra*x3)/kSc;
	out.velocity_km_per_day[2]=
		(-pwra*xp1+qwra*xp2+(pw2+qw2-1.0)*xp3-ppwra*x1+qpwra*x2+(ppw2+qpw2)*x3)/
		kSc;
}

}
